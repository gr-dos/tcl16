/*
 * main.c - tcl16's test driver for the candidate interpreters: Tiny Tcl
 * 6.8.1 (M1) and, built with TCL16_TCL67, the Tcl 6.7 DOS port (M2).
 *
 * This is the minimum needed to run the portable test scripts and the
 * memory probes; it is not the TCL.EXE command line, which is M4's
 * (docs/DESIGN.md, section 6):
 *
 *   TCL                  read commands from standard input, with the
 *                        "% " prompt and the continuation of incomplete
 *                        commands of upstream's tinytcl.c
 *   TCL FILE [ARG...]    evaluate FILE, with argv0, argv (a list) and
 *                        argc set; exit status 1 if it fails, with the
 *                        error and errorInfo on standard error
 *
 * Unlike upstream's tinytcl.c it does not print a banner or source
 * tcl_sys/autoinit.tcl, and it leaves out the "echo" and "checkmem"
 * test commands. For Tcl 6.7, Tcl_CreateInterp makes every command but
 * "history", which Tcl_RecordAndEval makes when it first records a
 * command: the driver makes it at the start, and records interactive
 * commands, as upstream's tclTest.c does; like tclTest.c, it sources no
 * init.tcl.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"

#ifndef TCL16_TCL67
/* Initialisers of Tiny Tcl's TclX and readdir commands (tclXdbg.c,
 * tclXgen.c, readdir.c); upstream declares them nowhere usable. */
void Tcl_InitDebug(Tcl_Interp *interp);
void TclX_InitGeneral(Tcl_Interp *interp);
int Tcl_InitReaddir(Tcl_Interp *interp);
#endif
#ifdef TCL16_PROBE
/* m1probe, the M1 measurement command (probe.c; dev builds only). */
int Tcl_InitProbe(Tcl_Interp *interp);
#endif

static Tcl_Interp *
make_interp(void)
{
    Tcl_Interp *interp = Tcl_CreateInterp();

#ifndef TCL16_TCL67
    Tcl_InitDebug(interp);
    TclX_InitGeneral(interp);
    Tcl_InitReaddir(interp);
#else
    Tcl_InitHistory(interp);
#endif
#ifdef TCL16_PROBE
    Tcl_InitProbe(interp);
#endif
    return interp;
}

static int
run_file(Tcl_Interp *interp, int argc, char **argv)
{
    char *args;
    char count[8];
    char *info;

    args = Tcl_Merge(argc - 2, argv + 2);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(count, "%d", argc - 2);
    Tcl_SetVar(interp, "argc", count, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", argv[1], TCL_GLOBAL_ONLY);

    if (Tcl_EvalFile(interp, argv[1]) == TCL_OK) {
        return 0;
    }
    info = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    fprintf(stderr, "Error: %s\n", interp->result);
    if (info != NULL && *info != '\0') {
        fprintf(stderr, "%s\n", info);
    }
    return 1;
}

static int
run_interactive(Tcl_Interp *interp)
{
    static char line[1000];
    Tcl_CmdBuf buffer = Tcl_CreateCmdBuf();
    int partial = 0;
    int result;
    char *cmd;

    for (;;) {
        clearerr(stdin);
        if (!partial) {
            fputs("% ", stdout);
            fflush(stdout);
        }
        if (fgets(line, sizeof line, stdin) == NULL) {
            if (!partial) {
                return 0;
            }
            line[0] = '\0';
        }
        cmd = Tcl_AssembleCmd(buffer, line);
        if (cmd == NULL) {
            partial = 1;
            continue;
        }
        partial = 0;
#ifdef TCL16_TCL67
        result = Tcl_RecordAndEval(interp, cmd, 0);
#else
        result = Tcl_Eval(interp, cmd, 0, (char **) NULL);
#endif
        if (result == TCL_OK) {
            if (*interp->result != '\0') {
                printf("%s\n", interp->result);
            }
        } else if (result == TCL_ERROR) {
            printf("Error: %s\n", interp->result);
        } else {
            printf("Error %d: %s\n", result, interp->result);
        }
    }
}

int
main(int argc, char **argv)
{
    Tcl_Interp *interp = make_interp();

    if (argc > 1) {
        return run_file(interp, argc, argv);
    }
    return run_interactive(interp);
}
