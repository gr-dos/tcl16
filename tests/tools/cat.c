/*
 * cat.c - CAT.EXE for Tcl 6.7's test suite (make test-suite;
 * scripts/run-tcl-suite): CAT [FILE...] copies each FILE, or standard
 * input, to standard output, byte for byte. Exits 0, or 1 after "cat:
 * FILE: MESSAGE" on standard error for a file it cannot read.
 *
 * Tcl16's own, in place of the Tcl 6.7 DOS port's UTIL\CAT.EXE, which
 * ends its output with a ^Z. A test tool, built with libc16 in the small
 * model; never part of a release.
 */

#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <errno.h>

static char buf[1024];

static void
copy(int fd)
{
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
	write(1, buf, n);
    }
}

int
main(int argc, char **argv)
{
    int i, status = 0;

    setmode(0, O_BINARY);
    setmode(1, O_BINARY);
    if (argc < 2) {
	copy(0);
    }
    for (i = 1; i < argc; i++) {
	int fd = open(argv[i], O_RDONLY | O_BINARY);

	if (fd < 0) {
	    char *msg = strerror(errno);

	    write(2, "cat: ", 5);
	    write(2, argv[i], strlen(argv[i]));
	    write(2, ": ", 2);
	    write(2, msg, strlen(msg));
	    write(2, "\n", 1);
	    status = 1;
	    continue;
	}
	copy(fd);
	close(fd);
    }
    return status;
}
