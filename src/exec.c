/*
 * exec.c - the "exec" command, for DOS (docs/DESIGN.md, sections 8.1 and
 * 12; docs/COMPATIBILITY.md, "Running programs").
 *
 *   exec ?REDIRECTION ...? WORD ?WORD ...? ?| WORD ...? ...
 *
 * As in Tcl 6.7, exec runs a pipeline of commands, waits for them, and
 * returns what the last one writes to its standard output, less one
 * final newline. It is an error, with that output and then what the
 * commands wrote to standard error as its message, if any command exits
 * with a status other than 0 ($errorCode is then "CHILDSTATUS 0 STATUS")
 * or writes to an unredirected standard error. Redirections, anywhere in
 * the arguments, apply to the whole pipeline:
 *
 *   < FILE      the first command reads FILE ("<FILE" too)
 *   << VALUE    the first command reads VALUE
 *   > FILE      the last command writes FILE, created or emptied
 *   >> FILE     ... appended to
 *   >& FILE, >>& FILE   ... and every command's standard error with it
 *   |&          between commands: standard error is piped too
 *
 * and, as Tcl 7 later gave them, to use a file open in Tcl:
 *
 *   <@ FILEID   the first command reads FILEID (stdin for the console)
 *   >@ FILEID   the last command writes FILEID (stdout to show its
 *               output as it runs); >&@ FILEID with standard error
 *
 * On DOS:
 *
 * - The commands run one after another, not at once: a pipe is a
 *   temporary file, as COMMAND.COM makes it, in the directory TEMP (or
 *   TMP) names, or the current one. A trailing "&" (in the background)
 *   is an error.
 * - Without "<" or "<@", the first command reads NUL, never the console.
 * - A program is found as COMMAND.COM finds it: a name with no "\", "/"
 *   or ":" in the current directory, then in each directory of
 *   env(PATH); with no extension, NAME.COM, NAME.EXE then NAME.BAT. It
 *   runs with INT 21h 4B00h (libc16's spawnve), with the env array as
 *   its environment and its arguments quoted for libc16's parser. The
 *   command tail is at most 126 bytes.
 * - A .BAT file, and COMMAND.COM's own commands (DIR, COPY, DEL, ECHO,
 *   TYPE... below) when the name has no path or extension, run with
 *   "%COMSPEC% /C" (libc16_systeme), their words joined by spaces as
 *   they are, at most 122 bytes. COMMAND.COM does not give back their
 *   exit status: it counts as 0.
 * - The child gets the conventional memory TCL.EXE has not taken: its
 *   heap is never given back to DOS (DESIGN.md, section 12).
 * - The output exec returns is read as DOS text (CR LF and ^Z), and may
 *   be at most TCL16_EXEC_MAX bytes; redirect more to a file.
 */

#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tclInt.h"
#include "tcldos.h"
#include "tcl16.h"

/*
 * The most output (or standard error) exec reads back, in bytes.
 */

#define TCL16_EXEC_MAX 32000

/*
 * The longest DOS path, with its NUL.
 */

#define DOS_PATH_MAX 128

/*
 * COMMAND.COM's own commands, which exec runs with COMSPEC /C.
 */

static char *shellCommands[] = {
    "BREAK", "CD", "CHCP", "CHDIR", "CLS", "COPY", "CTTY", "DATE", "DEL",
    "DIR", "ECHO", "ERASE", "LH", "LOADHIGH", "MD", "MKDIR", "PATH",
    "PAUSE", "PROMPT", "RD", "REM", "REN", "RENAME", "RMDIR", "SET",
    "TIME", "TRUENAME", "TYPE", "VER", "VERIFY", "VOL", NULL
};

/*
 * A temporary file: its handle and name.
 */

typedef struct {
    int fd;			/* -1 when there is none. */
    char name[DOS_PATH_MAX];
} TempFile;

static unsigned tempCount;	/* Makes temporary names. */

static int
OpenTemp(Tcl_Interp *interp, TempFile *t)
{
    char *dir = Tcl_GetVar2(interp, "env", "TEMP", TCL_GLOBAL_ONLY);
    int len, tries;

    if (dir == NULL) {
	dir = Tcl_GetVar2(interp, "env", "TMP", TCL_GLOBAL_ONLY);
    }
    if (dir == NULL || strlen(dir) > DOS_PATH_MAX - 16) {
	dir = "";
    }
    len = strlen(dir);
    for (tries = 0; tries < 100; tries++) {
	strcpy(t->name, dir);
	if (len > 0 && !TCL16_IS_SEP(dir[len - 1]) && dir[len - 1] != ':') {
	    strcat(t->name, "\\");
	}
	sprintf(t->name + strlen(t->name), "TCL%05u.TMP", tempCount++);
	t->fd = open(t->name, O_RDWR | O_CREAT | O_EXCL | O_BINARY,
		S_IREAD | S_IWRITE);
	if (t->fd >= 0) {
	    return TCL_OK;
	}
	if (errno != EEXIST) {
	    break;
	}
    }
    Tcl_AppendResult(interp, "couldn't create temporary file \"", t->name,
	    "\": ", Tcl_UnixError(interp), (char *) NULL);
    t->fd = -1;
    return TCL_ERROR;
}

static void
CloseTemp(TempFile *t)
{
    if (t->fd >= 0) {
	close(t->fd);
	unlink(t->name);
	t->fd = -1;
    }
}

/*
 * Appends the contents of a temporary file to buf (whose length is
 * *lenPtr), as DOS text; TCL_ERROR if the total would pass
 * TCL16_EXEC_MAX.
 */

static int
ReadTemp(Tcl_Interp *interp, TempFile *t, char *buf, int *lenPtr)
{
    long size;
    int n;

    if (t->fd < 0) {
	return TCL_OK;
    }
    size = lseek(t->fd, 0L, SEEK_END);
    if (size < 0 || size > (long) (TCL16_EXEC_MAX - *lenPtr)) {
	Tcl_AppendResult(interp, "the output of exec is too long ",
		"(the most is 32000 bytes): redirect it to a file",
		(char *) NULL);
	return TCL_ERROR;
    }
    lseek(t->fd, 0L, SEEK_SET);
    n = read(t->fd, buf + *lenPtr, (unsigned) size);
    if (n < 0) {
	n = 0;
    }
    *lenPtr += Tcl16_DosText(buf + *lenPtr, n);
    return TCL_OK;
}

/*
 * How a command runs.
 */

#define RUN_PROGRAM	0	/* path is a program: spawnve. */
#define RUN_SHELL	1	/* COMSPEC /C. */
#define RUN_MISSING	2	/* Not found. */

static int
IsFile(const char *path)
{
    unsigned attr;

    return _dos_getfileattr(path, &attr) == 0
	    && (attr & (_A_SUBDIR | _A_VOLID)) == 0;
}

/*
 * Whether dir followed by name (and an extension) is a file; stores the
 * full name in path.
 */

static int
TryName(const char *dir, int dirLen, const char *name, const char *ext,
	char *path)
{
    int len = dirLen, nameLen = strlen(name);

    if (dirLen + 1 + nameLen + strlen(ext) + 1 > DOS_PATH_MAX) {
	return 0;
    }
    memcpy(path, dir, dirLen);
    if (dirLen > 0 && !TCL16_IS_SEP(dir[dirLen - 1])
	    && dir[dirLen - 1] != ':') {
	path[len++] = '\\';
    }
    strcpy(path + len, name);
    strcat(path, ext);
    return IsFile(path);
}

/*
 * Finds the program "name" names, as COMMAND.COM would; stores its
 * full name in path, or "" for one of COMMAND.COM's own commands.
 */

static int
FindProgram(Tcl_Interp *interp, char *name, char *path)
{
    static char *exts[] = {".COM", ".EXE", ".BAT", NULL};
    char *tail = Tcl16_PathTail(name), *dirs, *ext;
    int hasPath = (tail != name), hasExt = (strchr(tail, '.') != NULL);
    int i;

    path[0] = 0;
    if (!hasPath && !hasExt) {
	for (i = 0; shellCommands[i] != NULL; i++) {
	    if (stricmp(name, shellCommands[i]) == 0) {
		return RUN_SHELL;
	    }
	}
    }

    dirs = hasPath ? NULL : Tcl_GetVar2(interp, "env", "PATH",
	    TCL_GLOBAL_ONLY);
    for (i = -1; ; ) {
	const char *dir = "";
	int dirLen = 0, e;

	/*
	 * The current directory (or name's own path) first, then each
	 * element of PATH.
	 */

	if (i >= 0) {
	    if (dirs == NULL || dirs[i] == 0) {
		break;
	    }
	    dir = dirs + i;
	    while (dirs[i] != 0 && dirs[i] != ';') {
		i++;
	    }
	    dirLen = (dirs + i) - dir;
	    if (dirs[i] == ';') {
		i++;
	    }
	    if (dirLen == 0) {
		continue;
	    }
	} else {
	    i = 0;
	}
	if (hasExt) {
	    if (TryName(dir, dirLen, name, "", path)) {
		ext = strrchr(path, '.');
		return (stricmp(ext, ".BAT") == 0) ? RUN_SHELL : RUN_PROGRAM;
	    }
	} else {
	    for (e = 0; exts[e] != NULL; e++) {
		if (TryName(dir, dirLen, name, exts[e], path)) {
		    return (e == 2) ? RUN_SHELL : RUN_PROGRAM;
		}
	    }
	}
	if (hasPath) {
	    break;
	}
    }
    return RUN_MISSING;
}

/*
 * Puts back standard input, output and error from save[], the handles
 * RedirectStd saved, closing them. A handle not saved (-1) was never
 * changed. If DOS cannot put a handle back there is nothing better to
 * do than go on: the saved copy is closed either way.
 */

static void
RestoreStd(int save[3])
{
    int i;

    for (i = 0; i < 3; i++) {
	if (save[i] >= 0) {
	    dup2(save[i], i);
	    close(save[i]);
	    save[i] = -1;
	}
    }
}

/*
 * Makes handles in, out and err standard input, output and error,
 * saving the old ones in save[] for RestoreStd. Returns 0, or -1 with
 * errno, having put back whatever it had changed: all three handles are
 * saved before any is changed, so a failure (no free handle, EMFILE)
 * leaves Tcl's own standard handles as they were.
 */

static int
RedirectStd(int in, int out, int err, int save[3])
{
    int fds[3], i, e;

    fds[0] = in;
    fds[1] = out;
    fds[2] = err;
    for (i = 0; i < 3; i++) {
	save[i] = -1;
    }
    for (i = 0; i < 3; i++) {
	save[i] = dup(i);
	if (save[i] < 0) {
	    goto failed;
	}
    }
    for (i = 0; i < 3; i++) {
	if (dup2(fds[i], i) != 0) {
	    goto failed;
	}
    }
    return 0;

  failed:
    e = errno;
    RestoreStd(save);
    errno = e;
    return -1;
}

/*
 * Runs one command, words[0..n-1], with handles in, out and err as its
 * standard input, output and error. Stores its exit status. TCL_ERROR,
 * with a message, if it could not be run; then nothing was run, and
 * Tcl's standard handles are as they were.
 */

static int
RunCommand(Tcl_Interp *interp, char **words, int n, int in, int out,
	int err, int *statusPtr)
{
    char path[DOS_PATH_MAX], *saved;
    int how, save[3], i, code = 0, spawnErrno = 0;

    how = FindProgram(interp, words[0], path);
    if (how == RUN_MISSING) {
	/*
	 * As in Tcl 6.7, where the forked child reported it: a message
	 * on the command's standard error, and status 1. The name is
	 * written as it is, however long, and is not changed.
	 */

	static char before[] = "couldn't find \"", after[] = "\" to execute\n";

	write(err, before, sizeof(before) - 1);
	write(err, words[0], strlen(words[0]));
	write(err, after, sizeof(after) - 1);
	*statusPtr = 1;
	return TCL_OK;
    }

    fflush(stdout);
    fflush(stderr);
    if (RedirectStd(in, out, err, save) != 0) {
	Tcl_AppendResult(interp, "couldn't redirect standard input or ",
		"output for \"", words[0], "\": ", Tcl_UnixError(interp),
		(char *) NULL);
	return TCL_ERROR;
    }

    if (how == RUN_PROGRAM) {
	saved = words[n];
	words[n] = NULL;
	code = spawnve(P_WAIT, path, words, TclEnviron());
	words[n] = saved;
    } else {
	/*
	 * COMSPEC /C, the words as they are: a .BAT by its full name.
	 */

	char cmd[DOS_PATH_MAX];
	int len = 0;

	for (i = 0; i < n; i++) {
	    char *w = (i == 0 && path[0] != 0) ? path : words[i];
	    int wlen = strlen(w);

	    if (len + wlen + 1 >= (int) sizeof(cmd)) {
		code = -1;
		errno = E2BIG;
		break;
	    }
	    if (i > 0) {
		cmd[len++] = ' ';
	    }
	    memcpy(cmd + len, w, wlen);
	    len += wlen;
	}
	cmd[len] = 0;
	if (code == 0) {
	    code = libc16_systeme(cmd, TclEnviron());
	    if (code > 0) {
		code = 0;	/* COMMAND.COM's status is not the command's. */
	    }
	}
    }
    spawnErrno = errno;
    RestoreStd(save);

    if (code < 0) {
	errno = spawnErrno;
	Tcl_AppendResult(interp, "couldn't execute \"", words[0], "\": ",
		Tcl_UnixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    *statusPtr = code;
    return TCL_OK;
}

/*
 * The handle of a Tcl file id, checked for reading or writing.
 */

static int
FileIdHandle(Tcl_Interp *interp, char *id, int write)
{
    OpenFile *filePtr;

    if (TclGetOpenFile(interp, id, &filePtr) != TCL_OK) {
	return -1;
    }
    if (write ? !filePtr->writable : !filePtr->readable) {
	Tcl_AppendResult(interp, "\"", id, write ? "\" wasn't opened for writing"
		: "\" wasn't opened for reading", (char *) NULL);
	return -1;
    }
    if (write) {
	fflush(filePtr->f);
    }
    return fileno(filePtr->f);
}

	/* ARGSUSED */
int
Tcl16_ExecCmd(dummy, interp, argc, argv)
    ClientData dummy;
    Tcl_Interp *interp;
    int argc;
    char **argv;
{
    char **words;			/* Command words, NULL after each
					 * command. */
    int *starts, *errPiped;		/* Each command's first word, and
					 * whether its standard error goes
					 * with its output. */
    int numWords = 0, numCmds = 0, cmdStart = 0, i;
    char *inFile = NULL, *inValue = NULL, *inId = NULL;
    char *outFile = NULL, *outId = NULL;
    int outAppend = 0, outErr = 0;
    int inFd = -1, outFd = -1, closeIn = 0, closeOut = 0;
    TempFile inTemp, outTemp, errTemp, pipeIn, pipeOut;
    int result = TCL_ERROR, failed = 0;
    char *buf = NULL;
    int bufLen = 0, errLen;

    inTemp.fd = outTemp.fd = errTemp.fd = pipeIn.fd = pipeOut.fd = -1;
    words = (char **) ckalloc((unsigned) ((argc + 1) * sizeof(char *)));
    starts = (int *) ckalloc((unsigned) (argc * sizeof(int)));
    errPiped = (int *) ckalloc((unsigned) (argc * sizeof(int)));

    /*
     * Read the arguments: the commands' words and the redirections.
     */

    for (i = 1; i < argc; i++) {
	char *a = argv[i], **target = NULL;
	int skip = 0;

	if (a[0] == '|' && (a[1] == 0 || (a[1] == '&' && a[2] == 0))) {
	    if (numWords == cmdStart) {
		goto badPipe;
	    }
	    errPiped[numCmds] = (a[1] == '&');
	    starts[numCmds++] = cmdStart;
	    words[numWords++] = NULL;
	    cmdStart = numWords;
	    continue;
	}
	if (a[0] == '<') {
	    if (a[1] == '<') {
		target = &inValue;
		skip = 2;
	    } else if (a[1] == '@') {
		target = &inId;
		skip = 2;
	    } else {
		target = &inFile;
		skip = 1;
	    }
	    inFile = inValue = inId = NULL;
	} else if (a[0] == '>') {
	    char *p = a + 1;

	    outAppend = (*p == '>');
	    if (outAppend) {
		p++;
	    }
	    outErr = (*p == '&');
	    if (outErr) {
		p++;
	    }
	    outFile = outId = NULL;
	    if (*p == '@' && !outAppend) {
		target = &outId;
		p++;
	    } else {
		target = &outFile;
	    }
	    skip = p - a;
	} else if (a[0] == '&' && a[1] == 0 && i == argc - 1) {
	    Tcl_AppendResult(interp, "can't run a command in the ",
		    "background (\"&\") on DOS", (char *) NULL);
	    goto done;
	}
	if (target != NULL) {
	    if (a[skip] != 0) {
		*target = a + skip;
	    } else if (i + 1 < argc) {
		*target = argv[++i];
	    } else {
		Tcl_AppendResult(interp, "can't specify \"", a,
			"\" as last word in command", (char *) NULL);
		goto done;
	    }
	    continue;
	}
	words[numWords++] = a;
    }
    if (numWords == 0) {
	interp->result = "didn't specify command to execute";
	goto done;
    }
    if (numWords == cmdStart) {
	badPipe:
	interp->result = "illegal use of | in command";
	goto done;
    }
    errPiped[numCmds] = 0;
    starts[numCmds++] = cmdStart;
    words[numWords] = NULL;

    /*
     * Open the pipeline's input, output and standard error.
     */

    if (inFile != NULL) {
	inFd = open(inFile, O_RDONLY | O_BINARY);
	if (inFd < 0) {
	    Tcl_AppendResult(interp, "couldn't read file \"", inFile, "\": ",
		    Tcl_UnixError(interp), (char *) NULL);
	    goto done;
	}
	closeIn = 1;
    } else if (inValue != NULL) {
	int len = strlen(inValue);

	if (OpenTemp(interp, &inTemp) != TCL_OK) {
	    goto done;
	}
	if (write(inTemp.fd, inValue, len) != len) {
	    Tcl_AppendResult(interp, "couldn't write file \"", inTemp.name,
		    "\": ", Tcl_UnixError(interp), (char *) NULL);
	    goto done;
	}
	lseek(inTemp.fd, 0L, SEEK_SET);
	inFd = inTemp.fd;
    } else if (inId != NULL) {
	inFd = FileIdHandle(interp, inId, 0);
	if (inFd < 0) {
	    goto done;
	}
    } else {
	inFd = open("NUL", O_RDONLY | O_BINARY);
	if (inFd < 0) {
	    Tcl_AppendResult(interp, "couldn't read file \"NUL\": ",
		    Tcl_UnixError(interp), (char *) NULL);
	    goto done;
	}
	closeIn = 1;
    }

    if (outFile != NULL) {
	outFd = open(outFile, O_WRONLY | O_CREAT | O_BINARY
		| (outAppend ? O_APPEND : O_TRUNC), S_IREAD | S_IWRITE);
	if (outFd < 0) {
	    Tcl_AppendResult(interp, "couldn't write file \"", outFile,
		    "\": ", Tcl_UnixError(interp), (char *) NULL);
	    goto done;
	}
	closeOut = 1;
	if (outAppend) {
	    /*
	     * DOS handles have no append mode (O_APPEND is libc16's own,
	     * for its write): the child writes at the file pointer.
	     */

	    lseek(outFd, 0L, SEEK_END);
	}
    } else if (outId != NULL) {
	outFd = FileIdHandle(interp, outId, 1);
	if (outFd < 0) {
	    goto done;
	}
    } else {
	if (OpenTemp(interp, &outTemp) != TCL_OK) {
	    goto done;
	}
	outFd = outTemp.fd;
    }
    if (OpenTemp(interp, &errTemp) != TCL_OK) {
	goto done;
    }

    /*
     * Run the commands in turn, each reading what the last wrote.
     */

    for (i = 0; i < numCmds; i++) {
	int last = (i == numCmds - 1), in, out, err, status;
	int n = (last ? numWords : starts[i + 1] - 1) - starts[i];


	in = (i == 0) ? inFd : pipeIn.fd;
	if (last) {
	    out = outFd;
	} else {
	    if (OpenTemp(interp, &pipeOut) != TCL_OK) {
		goto done;
	    }
	    out = pipeOut.fd;
	}
	err = ((last && outErr) || (!last && errPiped[i])) ? out : errTemp.fd;
	if (RunCommand(interp, words + starts[i], n, in, out, err, &status)
		!= TCL_OK) {
	    goto done;
	}
	if (status != 0) {
	    char code[8];

	    sprintf(code, "%d", status);
	    Tcl_SetErrorCode(interp, "CHILDSTATUS", "0", code, (char *) NULL);
	    failed = 1;
	}
	CloseTemp(&pipeIn);
	if (!last) {
	    pipeIn = pipeOut;
	    pipeOut.fd = -1;
	    lseek(pipeIn.fd, 0L, SEEK_SET);
	}
    }

    /*
     * The result: the output, then standard error, less a final newline.
     */

    buf = ckalloc(TCL16_EXEC_MAX + 1);
    if (ReadTemp(interp, &outTemp, buf, &bufLen) != TCL_OK) {
	goto done;
    }
    errLen = bufLen;
    if (ReadTemp(interp, &errTemp, buf, &bufLen) != TCL_OK) {
	goto done;
    }
    if (bufLen > errLen) {
	failed = 1;
    }
    if (bufLen > 0 && buf[bufLen - 1] == '\n') {
	bufLen--;
    }
    buf[bufLen] = 0;
    Tcl_SetResult(interp, buf, TCL_DYNAMIC);
    buf = NULL;
    result = failed ? TCL_ERROR : TCL_OK;

  done:
    if (buf != NULL) {
	ckfree(buf);
    }
    if (closeIn) {
	close(inFd);
    }
    if (closeOut) {
	close(outFd);
    }
    CloseTemp(&inTemp);
    CloseTemp(&outTemp);
    CloseTemp(&errTemp);
    CloseTemp(&pipeIn);
    CloseTemp(&pipeOut);
    ckfree((char *) words);
    ckfree((char *) starts);
    ckfree((char *) errPiped);
    return result;
}
