/*
 * main.c - TCL.EXE's command line (docs/DESIGN.md, section 6;
 * docs/COMPATIBILITY.md, "The TCL.EXE command line").
 *
 *   TCL                     interactive: read commands from standard
 *                           input, with the "% " prompt, "> " while a
 *                           command is incomplete
 *   TCL SCRIPT [ARG...]     evaluate SCRIPT, with argv0, argv and argc set
 *   TCL -e TEXT [ARG...]    evaluate TEXT, with argv0 "-e" (a Tcl16
 *                           extension)
 *   TCL -- SCRIPT [ARG...]  SCRIPT even if it starts with "-"
 *   TCL /?  (or -?, -h)     usage
 *
 * Exit status: 0 when the script (or the REPL, at the end of its input)
 * ends normally; that of "exit N"; 1 for an uncaught error, or a
 * command left incomplete at the end of the REPL's input; 2 for a bad
 * command line; 255 when Tcl16 stops for want of memory (panic).
 * Diagnostics go to standard error, after standard output is flushed.
 *
 * Startup: TCL.EXE prints no banner, sources no startup script (Tiny
 * Tcl's tcl_sys/autoinit.tcl needs its Borland-only commands), and sets
 * no tcl_library; "info library" reports the TCL_LIBRARY environment
 * variable, if set, as in Tcl 6.7.
 *
 * The development build's measurement command, m1probe (probe.c), is
 * registered only with TCL16_PROBE, which only scripts/build-variant
 * defines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "tclInt.h"
#include "tcl16.h"

/* Initialisers of Tiny Tcl's TclX and readdir commands (tclXdbg.c,
 * tclXgen.c, readdir.c); upstream declares them nowhere usable. */
void Tcl_InitDebug(Tcl_Interp *interp);
void TclX_InitGeneral(Tcl_Interp *interp);
int Tcl_InitReaddir(Tcl_Interp *interp);
/* Tcl16's exec (exec.c). */
int Tcl16_ExecCmd(ClientData dummy, Tcl_Interp *interp, int argc,
    char **argv);
#ifdef TCL16_PROBE
/* m1probe, the M1 measurement command (probe.c; measurement builds). */
int Tcl_InitProbe(Tcl_Interp *interp);
#endif

#define EXIT_ERROR 1		/* An uncaught Tcl error. */
#define EXIT_USAGE 2		/* A bad command line. */

static char usage[] =
"Tcl16 " TCL16_VERSION ": Tcl 6.7 (Tiny Tcl 6.8.1) for DOS\n"
"\n"
"TCL                     interactive; end with EXIT or Ctrl-Z\n"
"TCL script [arg ...]    run a script; argv0, argv and argc are set\n"
"TCL -e text [arg ...]   run the Tcl commands in text (argv0 is -e)\n"
"TCL -- script [arg ...] run a script whose name starts with -\n"
"TCL /?                  this help\n"
"\n"
"Exit status: 0, or N for \"exit N\"; 1 after an uncaught error;\n"
"2 for a bad command line.\n";

static Tcl_Interp *
MakeInterp(void)
{
    Tcl_Interp *interp = Tcl_CreateInterp();

    Tcl_InitDebug(interp);
    TclX_InitGeneral(interp);
    Tcl_InitReaddir(interp);
    Tcl_CreateCommand(interp, "exec", Tcl16_ExecCmd, (ClientData) NULL,
	    (Tcl_CmdDeleteProc *) NULL);
#ifdef TCL16_PROBE
    Tcl_InitProbe(interp);
#endif
    return interp;
}

/*
 * Diagnostic: write a message to standard error, after what is already
 * on standard output, so that the two keep their order when they go to
 * the same place.
 */

static void
Diagnostic(char *prefix, char *message)
{
    fflush(stdout);
    fputs(prefix, stderr);
    fputs(message, stderr);
    fputc('\n', stderr);
    fflush(stderr);
}

/*
 * CodeMessage: the result of a command that ended with a code other than
 * TCL_OK, TCL_ERROR or TCL_RETURN at the top level, as Tcl reports it:
 * break and continue outside a loop are errors.
 */

static void
CodeMessage(Tcl_Interp *interp, int code)
{
    char msg[48];

    if (code == TCL_BREAK) {
	strcpy(msg, "invoked \"break\" outside of a loop");
    } else if (code == TCL_CONTINUE) {
	strcpy(msg, "invoked \"continue\" outside of a loop");
    } else {
	sprintf(msg, "command returned bad code: %d", code);
    }
    Tcl_SetResult(interp, msg, TCL_VOLATILE);
}

/*
 *----------------------------------------------------------------------
 *
 * ReportUncaught --
 *
 *	Report a script's uncaught error on standard error: "Error: ",
 *	then errorInfo, which starts with the message and says where the
 *	error happened.
 *
 * Results:
 *	EXIT_ERROR.
 *
 *----------------------------------------------------------------------
 */

static int
ReportUncaught(Tcl_Interp *interp, int code)
{
    char *info;

    if (code != TCL_ERROR) {
	CodeMessage(interp, code);
    }

    /*
     * Tcl_AddErrorInfo starts errorInfo from the result if no error is
     * being logged: when the script could not be read, for example.
     */

    Tcl_AddErrorInfo(interp, "");
    info = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    Diagnostic("Error: ", info != NULL ? info : interp->result);
    return EXIT_ERROR;
}

/*
 * SetArgs: argv0, and argv and argc from the arguments after it.
 */

static void
SetArgs(Tcl_Interp *interp, char *argv0, int argc, char **argv)
{
    char *args;
    char count[8];

    args = Tcl_Merge(argc, argv);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(count, "%d", argc);
    Tcl_SetVar(interp, "argc", count, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", argv0, TCL_GLOBAL_ONLY);
}

/*
 *----------------------------------------------------------------------
 *
 * RunScript, RunText --
 *
 *	TCL SCRIPT and TCL -e TEXT: evaluate the script, which "return"
 *	ends, and report an uncaught error.
 *
 * Results:
 *	The exit status: 0 or EXIT_ERROR.
 *
 *----------------------------------------------------------------------
 */

static int
RunScript(Tcl_Interp *interp, char *fileName)
{
    int code = Tcl_EvalFile(interp, fileName);

    return (code == TCL_OK) ? 0 : ReportUncaught(interp, code);
}

static int
RunText(Tcl_Interp *interp, char *text)
{
    char *end;
    int code = Tcl_Eval(interp, text, 0, &end);

    if (code == TCL_RETURN) {
	code = TCL_OK;
    }
    return (code == TCL_OK) ? 0 : ReportUncaught(interp, code);
}

/*
 *----------------------------------------------------------------------
 *
 * Interactive --
 *
 *	TCL: read commands from standard input as DOS text, line by line,
 *	and evaluate each when it is complete; print its result on
 *	standard output, or its error on standard error, and go on. A
 *	command too long for Tcl_AssembleCmd is dropped with an error. The
 *	end of the input (Ctrl-Z on the console, or the end of a
 *	redirected file) ends the session.
 *
 * Results:
 *	The exit status: 0, or EXIT_ERROR if the input ends inside a
 *	command or cannot be read.
 *
 *----------------------------------------------------------------------
 */

static int
FillFromStdin(void *data, char *buf, int size)
{
    (void) data;
    if (fgets(buf, size, stdin) == NULL) {
	return ferror(stdin) ? -1 : 0;
    }
    return strlen(buf);
}

static int
Interactive(Tcl_Interp *interp)
{
    Tcl16Reader reader;
    Tcl_CmdBuf buffer = Tcl_CreateCmdBuf();
    int pending = 0;		/* Bytes of the command assembled so far. */
    int status = 0;
    int length, code;
    char *cmd, *end;
    char tooLong[48];

    Tcl16_InitReader(&reader, FillFromStdin, NULL);
    while (1) {
	fputs(pending ? "> " : "% ", stdout);
	fflush(stdout);
	length = Tcl16_ReadLine(&reader, TCL16_MAX_COMMAND - pending);
	if (length == TCL16_READ_EOF) {
	    if (pending) {
		Diagnostic("Error: ", "incomplete command at end of input");
		status = EXIT_ERROR;
	    }
	    break;
	}
	if (length == TCL16_READ_ERROR) {
	    Diagnostic("Error: reading standard input: ", strerror(errno));
	    status = EXIT_ERROR;
	    break;
	}
	if (length == TCL16_READ_TOO_LONG) {
	    sprintf(tooLong, "command too long (the most is %d bytes)",
		    TCL16_MAX_COMMAND);
	    Diagnostic("Error: ", tooLong);
	    (void) Tcl_AssembleCmd(buffer, "");	/* Drop what came before. */
	    pending = 0;
	    continue;
	}
	if (reader.line[length - 1] != '\n') {
	    /* A last line with no LF: complete it. */
	    reader.line[length++] = '\n';
	    reader.line[length] = '\0';
	}
	cmd = Tcl_AssembleCmd(buffer, reader.line);
	if (cmd == NULL) {
	    pending += length;
	    continue;
	}
	pending = 0;
	code = Tcl_Eval(interp, cmd, 0, &end);
	if (code == TCL_OK || code == TCL_RETURN) {
	    if (*interp->result != '\0') {
		fputs(interp->result, stdout);
		fputc('\n', stdout);
	    }
	    continue;
	}
	if (code != TCL_ERROR) {
	    CodeMessage(interp, code);
	}
	Diagnostic("Error: ", interp->result);
    }
    Tcl16_FreeReader(&reader);
    Tcl_DeleteCmdBuf(buffer);
    return status;
}

static int
Usage(char *problem, char *arg)
{
    fflush(stdout);
    fputs("TCL: ", stderr);
    fputs(problem, stderr);
    fputs(arg, stderr);
    fputs("\nType TCL /? for help.\n", stderr);
    return EXIT_USAGE;
}

int
main(int argc, char **argv)
{
    Tcl_Interp *interp;
    char *arg = (argc > 1) ? argv[1] : NULL;
    int status;

    if (arg != NULL && (strcmp(arg, "/?") == 0 || strcmp(arg, "-?") == 0
	    || strcmp(arg, "-h") == 0)) {
	fputs(usage, stdout);
	return 0;
    }
    if (arg != NULL && arg[0] == '-' && strcmp(arg, "-e") != 0
	    && strcmp(arg, "--") != 0) {
	return Usage("unknown option ", arg);
    }
    if (arg != NULL && arg[0] == '-' && argc < 3) {
	return Usage("missing argument after ", arg);
    }

    interp = MakeInterp();
    if (arg == NULL) {
	SetArgs(interp, argv[0], 0, argv + 1);
	status = Interactive(interp);
    } else if (strcmp(arg, "-e") == 0) {
	SetArgs(interp, "-e", argc - 3, argv + 3);
	status = RunText(interp, argv[2]);
    } else {
	int first = (strcmp(arg, "--") == 0) ? 2 : 1;

	SetArgs(interp, argv[first], argc - first - 1, argv + first + 1);
	status = RunScript(interp, argv[first]);
    }
    fflush(stdout);
    return status;
}
