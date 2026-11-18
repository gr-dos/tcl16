/*
 * pwd.c - PWD.EXE for Tcl 6.7's test suite (make test-suite;
 * scripts/run-tcl-suite): writes the current directory, with "/" for
 * "\" as the Tcl 6.7 DOS port's UTIL\PWD.EXE wrote it, and a newline,
 * and exits 0.
 *
 * Tcl16's own, in place of that PWD.EXE. A test tool, built with libc16
 * in the small model; never part of a release.
 */

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>

int
main(void)
{
    static char buf[80];
    char *p;

    setmode(1, O_BINARY);
    if (getcwd(buf, sizeof(buf)) == NULL) {
	return 1;
    }
    for (p = buf; *p != 0; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }
    write(1, buf, strlen(buf));
    write(1, "\r\n", 2);
    return 0;
}
