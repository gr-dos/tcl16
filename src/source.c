/*
 * source.c - Tcl_EvalFile for Tcl16: "source" and TCL.EXE's script.
 *
 * Tcl16's own replacement for Tiny Tcl's Tcl_EvalFile and
 * Tcl_SmallFootprintEvalFile (tcldosut.c, removed there by
 * patches/0009-dos-source.patch). Tiny Tcl read the script in binary
 * mode, so a DOS text script, with CR LF line ends and perhaps a ^Z at
 * its end, failed: a backslash before CR LF did not continue the line,
 * and the ^Z ran as a command (tests/tinytcl/crlf.tcl, ctrlz.tcl). Here
 * the script is read as DOS text (dostext.c).
 *
 * A script that fits in memory is read whole and evaluated as one Tcl
 * script, as Tcl does. A larger one, of INT_MAX bytes or more, which no
 * Tcl string can hold, or one malloc cannot hold, is read line by line
 * and each command evaluated as soon as it is complete, as Tiny Tcl
 * did; then only each command must fit within Tcl's limits
 * (TCL16_MAX_COMMAND). Either way:
 *
 *   - "return" ends the script, and "source" returns its value;
 *   - break, continue and errors end it with that code;
 *   - an error adds '(file "NAME" line N)' to errorInfo, N counted from
 *     the start of the file.
 */

#include <stdlib.h>
#include <string.h>
#include "tclInt.h"
#include "tcldos.h"
#include "tcl16.h"

static int	EvalByCommand(Tcl_Interp *interp, char *fileName, int fd);
static int	FillFromFd(void *data, char *buf, int size);
static void	AddFileInfo(Tcl_Interp *interp, char *fileName, int line);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalFile --
 *
 *	Read a file of DOS text and evaluate it as a Tcl script.
 *
 * Results:
 *	A standard Tcl result: that of the script, or an error saying why
 *	the file could not be read.
 *
 * Side effects:
 *	Those of the script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_EvalFile(Tcl_Interp *interp, char *fileName)
{
    Interp *iPtr = (Interp *) interp;
    char *oldScriptFile = iPtr->scriptFile;
    struct stat statBuf;
    char *script = NULL;
    char *end;
    int fd, length, result;

    iPtr->scriptFile = fileName;
#ifdef TCL_TILDE_SUBST
    fileName = Tcl_TildeSubst(interp, fileName);
    if (fileName == NULL) {
	iPtr->scriptFile = oldScriptFile;
	return TCL_ERROR;
    }
#endif
    fd = open(fileName, O_RDONLY | O_BINARY, 0);
    if (fd < 0) {
	Tcl_AppendResult(interp, "couldn't read file \"", fileName,
		"\": ", Tcl_UnixError(interp), (char *) NULL);
	iPtr->scriptFile = oldScriptFile;
	return TCL_ERROR;
    }
    if (fstat(fd, &statBuf) == -1) {
	Tcl_AppendResult(interp, "couldn't stat file \"", fileName,
		"\": ", Tcl_UnixError(interp), (char *) NULL);
	close(fd);
	iPtr->scriptFile = oldScriptFile;
	return TCL_ERROR;
    }

    /*
     * malloc, not ckalloc, which panics when memory runs out: if it
     * fails, the script is read line by line instead.
     */

    if (statBuf.st_size < INT_MAX) {
	script = (char *) malloc((unsigned) statBuf.st_size + 1);
    }
    if (script == NULL) {
	result = EvalByCommand(interp, fileName, fd);
	close(fd);
	iPtr->scriptFile = oldScriptFile;
	return result;
    }
    length = read(fd, script, (unsigned) statBuf.st_size);
    if (length < 0) {
	Tcl_AppendResult(interp, "error in reading file \"", fileName,
		"\": ", Tcl_UnixError(interp), (char *) NULL);
	close(fd);
	free(script);
	iPtr->scriptFile = oldScriptFile;
	return TCL_ERROR;
    }
    close(fd);
    Tcl16_DosText(script, length);
    result = Tcl_Eval(interp, script, 0, &end);
    if (result == TCL_RETURN) {
	result = TCL_OK;
    } else if (result == TCL_ERROR) {
	AddFileInfo(interp, fileName, interp->errorLine);
    }
    free(script);
    iPtr->scriptFile = oldScriptFile;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * EvalByCommand --
 *
 *	Evaluate the script open on "fd" command by command, reading it
 *	line by line: for a script too large to read whole.
 *
 * Results:
 *	A standard Tcl result, as Tcl_EvalFile's.
 *
 *----------------------------------------------------------------------
 */

static int
EvalByCommand(Tcl_Interp *interp, char *fileName, int fd)
{
    Tcl16Reader reader;
    Tcl_CmdBuf buffer = Tcl_CreateCmdBuf();
    int pending = 0;		/* Bytes of the command assembled so far. */
    int lineNumber = 0;		/* Lines read. */
    int firstLine = 1;		/* Line where the command starts. */
    int result = TCL_OK;
    int length;
    char *cmd, *end;

    Tcl_ResetResult(interp);
    Tcl16_InitReader(&reader, FillFromFd, (void *) &fd);
    while (1) {
	length = Tcl16_ReadLine(&reader, TCL16_MAX_COMMAND - pending);
	if (length == TCL16_READ_ERROR) {
	    Tcl_AppendResult(interp, "error in reading file \"", fileName,
		    "\": ", Tcl_UnixError(interp), (char *) NULL);
	    result = TCL_ERROR;
	    break;
	}
	if (length == TCL16_READ_TOO_LONG) {
	    char most[8];

	    sprintf(most, "%d", TCL16_MAX_COMMAND);
	    Tcl_AppendResult(interp, "command too long (the most is ",
		    most, " bytes)", (char *) NULL);
	    AddFileInfo(interp, fileName, firstLine);
	    result = TCL_ERROR;
	    break;
	}
	if (length == TCL16_READ_EOF) {
	    if (pending == 0) {
		break;
	    }

	    /*
	     * An incomplete command at the end: evaluate it anyway, as a
	     * whole script would be, for the parse error.
	     */

	    cmd = Tcl_AssembleCmd(buffer, "");
	} else {
	    lineNumber++;
	    if (pending == 0) {
		firstLine = lineNumber;
	    }
	    if (reader.line[length - 1] != '\n') {
		/* The last line, with no LF: complete it, as Tcl would. */
		reader.line[length++] = '\n';
		reader.line[length] = '\0';
	    }
	    cmd = Tcl_AssembleCmd(buffer, reader.line);
	    pending += length;
	}
	if (cmd == NULL) {
	    continue;
	}
	pending = 0;
	result = Tcl_Eval(interp, cmd, 0, &end);
	if (result == TCL_OK) {
	    continue;
	}
	if (result == TCL_RETURN) {
	    result = TCL_OK;
	} else if (result == TCL_ERROR) {
	    AddFileInfo(interp, fileName, firstLine + interp->errorLine - 1);
	}
	break;
    }
    Tcl16_FreeReader(&reader);
    Tcl_DeleteCmdBuf(buffer);
    return result;
}

static int
FillFromFd(void *data, char *buf, int size)
{
    return read(*(int *) data, buf, size);
}

/*
 * AddFileInfo: add where in the file an error happened to errorInfo.
 */

static void
AddFileInfo(Tcl_Interp *interp, char *fileName, int line)
{
    char msg[200];

    sprintf(msg, "\n    (file \"%.150s\" line %d)", fileName, line);
    Tcl_AddErrorInfo(interp, msg);
}
