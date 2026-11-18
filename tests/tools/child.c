/*
 * child.c - CHILD.EXE, the program tests/tcl16/exec*.tcl run with exec.
 * It reads its arguments in order:
 *
 *   -o TEXT   write TEXT and a newline (CR LF) to standard output
 *   -e TEXT   write TEXT and a newline to standard error
 *   -a        write each argument after it as [ARG], one per line
 *   -c        copy standard input to standard output
 *   -g NAME   write the value of the environment variable NAME, or
 *             (null)
 *   -m        write the size of the largest free block of conventional
 *             memory, in bytes (INT 21h 48h with BX = FFFFh)
 *   -x N      exit with status N (the last thing it does)
 *
 * A test tool, built with libc16 in the small model; never part of a
 * release.
 */

#include <dos.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <libc16.h>

static void
out(int fd, const char *s)
{
    write(fd, s, strlen(s));
}

int
main(int argc, char **argv)
{
    int i, status = 0;

    setmode(0, O_BINARY);	/* It writes its own CR LF. */
    setmode(1, O_BINARY);
    setmode(2, O_BINARY);
    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
	    out(1, argv[++i]);
	    out(1, "\r\n");
	} else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
	    out(2, argv[++i]);
	    out(2, "\r\n");
	} else if (strcmp(argv[i], "-a") == 0) {
	    for (i++; i < argc; i++) {
		out(1, "[");
		out(1, argv[i]);
		out(1, "]\r\n");
	    }
	} else if (strcmp(argv[i], "-c") == 0) {
	    char buf[256];
	    int n;

	    while ((n = read(0, buf, sizeof(buf))) > 0) {
		write(1, buf, n);
	    }
	} else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
	    char *v = getenv(argv[++i]);

	    out(1, v != NULL ? v : "(null)");
	    out(1, "\r\n");
	} else if (strcmp(argv[i], "-m") == 0) {
	    union REGS r;
	    char num[12];

	    r.h.ah = 0x48;
	    r.x.bx = 0xFFFF;
	    intdos(&r, &r);
	    out(1, libc16_ultoa((unsigned long) r.x.bx * 16, num, 10));
	    out(1, "\r\n");
	} else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
	    status = atoi(argv[++i]);
	}
    }
    return status;
}
