/*
 * dostext.c - DOS text for Tcl16: scripts, "source"d files and the
 * commands typed at the REPL, as DOS editors and the console make them.
 *
 * DOS text ends a line with CR LF, and may end with ^Z (1Ah), which DOS
 * editors and COPY /A append and which the console returns for Ctrl-Z.
 * Tcl expects LF alone: a CR before the LF would follow a backslash that
 * continues a line, and would be kept in a quoted or braced value that
 * spans lines. So, as Open Watcom's text mode and DOS's own tools do,
 * Tcl16 reads CR LF as LF, and the first ^Z as the end of the text.
 * A CR that is not before an LF is kept.
 *
 * Tcl16_DosText converts a whole buffer in place; a Tcl16Reader reads
 * lines from a file or standard input, converting each, for the commands
 * Tcl_AssembleCmd puts together line by line (source.c, main.c).
 */

#include <string.h>
#include "tclInt.h"
#include "tcl16.h"

#define CTRL_Z '\032'

/*
 * Bytes read from the input at a time, and the size a line buffer starts
 * at. Both come from the heap: Tcl16 keeps the stack for Tcl_Eval.
 */

#define CHUNK_SIZE 256
#define LINE_SIZE 128

/*
 *----------------------------------------------------------------------
 *
 * Tcl16_DosText --
 *
 *	Convert "length" bytes of DOS text at "buf" to Tcl's text, in
 *	place: stop at the first ^Z, and turn each CR LF into LF.
 *
 * Results:
 *	The new length; buf[length] is NUL. "buf" must have room for it.
 *
 *----------------------------------------------------------------------
 */

int
Tcl16_DosText(char *buf, int length)
{
    char *src, *dst, *end;

    end = memchr(buf, CTRL_Z, length);
    if (end == NULL) {
	end = buf + length;
    }
    for (src = dst = buf; src < end; src++) {
	if ((*src == '\r') && (src + 1 < end) && (src[1] == '\n')) {
	    continue;
	}
	*dst++ = *src;
    }
    *dst = '\0';
    return dst - buf;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl16_InitReader, Tcl16_FreeReader --
 *
 *	Start reading lines with "fill"; release what a reader holds.
 *
 *----------------------------------------------------------------------
 */

void
Tcl16_InitReader(Tcl16Reader *r, Tcl16FillProc fill, void *data)
{
    r->fill = fill;
    r->data = data;
    r->chunk = (char *) ckalloc(CHUNK_SIZE);
    r->chunkUsed = 0;
    r->chunkPos = 0;
    r->line = (char *) ckalloc(LINE_SIZE);
    r->line[0] = '\0';
    r->lineSize = LINE_SIZE;
    r->atEnd = 0;
}

void
Tcl16_FreeReader(Tcl16Reader *r)
{
    ckfree(r->chunk);
    ckfree(r->line);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl16_ReadLine --
 *
 *	Read the next line into r->line, as Tcl text: CR LF becomes LF, and
 *	a ^Z ends the input. The line keeps its LF, except a last line
 *	that has none; r->line has room for the caller to add an LF.
 *
 * Results:
 *	The line's length, or TCL16_READ_EOF when the input has ended
 *	(at its end or at a ^Z), TCL16_READ_ERROR if "fill" failed, or
 *	TCL16_READ_TOO_LONG if the line is more than "max" bytes; the rest
 *	of that line is then read and dropped, so that the next call reads
 *	the line after it.
 *
 *----------------------------------------------------------------------
 */

int
Tcl16_ReadLine(Tcl16Reader *r, int max)
{
    int used = 0;		/* Bytes in r->line. */
    int tooLong = 0;		/* The line is over max: drop the rest. */
    char c;

    while (1) {
	if (r->chunkPos >= r->chunkUsed) {
	    int n;

	    if (r->atEnd) {
		break;
	    }
	    n = (*r->fill)(r->data, r->chunk, CHUNK_SIZE);
	    if (n < 0) {
		r->atEnd = 1;
		return TCL16_READ_ERROR;
	    }
	    if (n == 0) {
		r->atEnd = 1;
		break;
	    }
	    r->chunkUsed = n;
	    r->chunkPos = 0;
	}
	c = r->chunk[r->chunkPos++];
	if (c == CTRL_Z) {
	    r->atEnd = 1;
	    r->chunkPos = r->chunkUsed;
	    break;
	}
	if (c == '\n' && used > 0 && r->line[used - 1] == '\r') {
	    used--;
	}
	if (!tooLong && used >= max) {
	    tooLong = 1;
	}
	if (tooLong) {
	    used = 0;
	    if (c == '\n') {
		return TCL16_READ_TOO_LONG;
	    }
	    continue;
	}
	if (used + 3 > r->lineSize) {
	    /* Room for c, and an LF and NUL the caller may add. */
	    int newSize = TclGrowSize((long) used + 3, 2L * r->lineSize);
	    char *newLine = (char *) ckalloc(newSize);

	    memcpy(newLine, r->line, used);
	    ckfree(r->line);
	    r->line = newLine;
	    r->lineSize = newSize;
	}
	r->line[used++] = c;
	if (c == '\n') {
	    break;
	}
    }
    if (tooLong) {
	return TCL16_READ_TOO_LONG;
    }
    r->line[used] = '\0';
    return used;
}
