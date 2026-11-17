/*
 * err2out.c - ERR2OUT PROGRAM ARGUMENTS: run PROGRAM with its standard
 * error going wherever standard output goes, and exit with its exit
 * status. COMMAND.COM cannot redirect standard error, so tests/run-dos
 * runs the cases marked "# stderr: merged" under it to see TCL.EXE's
 * diagnostics. ARGUMENTS are passed on exactly as typed, quotes and all.
 * Small model, so that TCL.EXE has nearly all of memory beside it.
 *
 * A test tool, built with Open Watcom's runtime like build/dev/TCL.EXE;
 * never part of a release. From gr-dos/perl16's tests/tools/err2out.c.
 */

#include <stdio.h>
#include <string.h>
#include <process.h>
#include <io.h>

int
main(void)
{
    static char line[130];
    char *prog, *args;
    int rc;

    getcmd(line);
    for (prog = line; *prog == ' ' || *prog == '\t'; prog++)
	;
    if (*prog == '\0') {
	fputs("usage: ERR2OUT PROGRAM ARGUMENTS\n", stderr);
	return 255;
    }
    for (args = prog; *args != '\0' && *args != ' ' && *args != '\t'; args++)
	;
    if (*args != '\0')
	*args++ = '\0';
    dup2(1, 2);
    rc = spawnl(P_WAIT, prog, prog, args, (char *)0);
    if (rc < 0) {
	perror(prog);
	return 255;
    }
    return rc;
}
