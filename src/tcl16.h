/*
 * tcl16.h - declarations shared by Tcl16's own source files (src/): DOS
 * text (dostext.c), "source" (source.c) and the command line (main.c).
 */

#ifndef TCL16_H
#define TCL16_H

#include <limits.h>

/*
 * Tcl16's own version, which /? reports. There is no release yet (M7
 * numbers the first). The language is Tcl 6.7 ("info tclversion"), as
 * implemented by Tiny Tcl 6.8.1.
 */

#define TCL16_VERSION "development build (M4)"

/*
 * The longest command TCL.EXE assembles from lines, from a script read
 * line by line or from standard input, in bytes: Tcl_AssembleCmd keeps
 * the command, an LF added to a last line that has none, and the
 * terminating NUL in one buffer of at most INT_MAX bytes
 * (patches/0008-assemble-size.patch).
 */

#define TCL16_MAX_COMMAND (INT_MAX - 2)

/*
 * A reader of DOS text, line by line (dostext.c). "fill" reads up to
 * "size" bytes into "buf" and returns how many it read, 0 at the end of
 * the input, or -1 on an error (errno set).
 */

typedef int (*Tcl16FillProc)(void *data, char *buf, int size);

typedef struct Tcl16Reader {
    Tcl16FillProc fill;		/* Reads more input. */
    void *data;			/* Passed to fill. */
    char *chunk;		/* Input read but not yet taken. */
    int chunkUsed;		/* Bytes in chunk. */
    int chunkPos;		/* Next byte of chunk to take. */
    char *line;			/* The last line read, NUL-terminated. */
    int lineSize;		/* Bytes allocated for line. */
    int atEnd;			/* The input has ended, or a ^Z was read. */
} Tcl16Reader;

#define TCL16_READ_EOF		0	/* No more lines. */
#define TCL16_READ_ERROR	(-1)	/* fill failed; errno says why. */
#define TCL16_READ_TOO_LONG	(-2)	/* Longer than allowed; skipped. */

extern void	Tcl16_InitReader(Tcl16Reader *r, Tcl16FillProc fill,
		    void *data);
extern void	Tcl16_FreeReader(Tcl16Reader *r);
extern int	Tcl16_ReadLine(Tcl16Reader *r, int max);
extern int	Tcl16_DosText(char *buf, int length);

#endif /* TCL16_H */
