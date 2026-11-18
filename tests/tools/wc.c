/*
 * wc.c - WC.EXE for Tcl 6.7's test suite (make test-suite;
 * scripts/run-tcl-suite): writes the number of words (runs of
 * characters other than white space) on standard input, and a newline,
 * and exits 0. The port's tests expect that one number, as the Tcl 6.7
 * DOS port's UTIL\WC.EXE wrote it (exec-1.3).
 *
 * Tcl16's own, in place of that WC.EXE, whose main returns no value, so
 * its exit status is whatever was left in AX. A test tool, built with
 * libc16 in the small model; never part of a release.
 */

#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <libc16.h>

int
main(void)
{
    static char buf[512];
    char num[12];
    unsigned long words = 0;
    int n, i, inWord = 0;

    setmode(0, O_BINARY);
    setmode(1, O_BINARY);
    while ((n = read(0, buf, sizeof(buf))) > 0) {
	for (i = 0; i < n; i++) {
	    if (isspace((unsigned char) buf[i]) || buf[i] == 0x1A) {
		inWord = 0;
	    } else if (!inWord) {
		inWord = 1;
		words++;
	    }
	}
    }
    libc16_ultoa(words, num, 10);
    write(1, num, strlen(num));
    write(1, "\r\n", 2);
    return 0;
}
