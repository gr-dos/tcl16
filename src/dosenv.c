/*
 * dosenv.c - the environment TCL.EXE was started with, as an environ
 * array, for Tiny Tcl's env array (tclEnv.c; patches/0010-libc16.patch).
 *
 * libc16 has getenv but no environ. Tcl16_Environ() reads the
 * environment block whose segment is at offset 2Ch of the PSP: strings
 * NAME=value, each ended by a NUL, the block ended by an empty string.
 * It returns a NULL-terminated array, made with malloc, of pointers into
 * that block (far pointers: the large model), or NULL when no memory is
 * left. A string with no "=" is left out, since the env array could not
 * name it. tclEnv.c copies the strings it keeps, and never writes to
 * the block.
 */

#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include "tcl16.h"

char **
Tcl16_Environ(void)
{
    unsigned envSeg = *(unsigned __far *) MK_FP(_psp, 0x2C);
    char *block = (char *) MK_FP(envSeg, 0);
    char *p, **env;
    int n = 0;

    if (envSeg != 0) {
	for (p = block; *p != 0; p += strlen(p) + 1) {
	    n++;
	}
    }
    env = (char **) malloc((n + 1) * sizeof(char *));
    if (env == NULL) {
	return NULL;
    }
    n = 0;
    if (envSeg != 0) {
	for (p = block; *p != 0; p += strlen(p) + 1) {
	    if (strchr(p, '=') != NULL) {
		env[n++] = p;
	    }
	}
    }
    env[n] = NULL;
    return env;
}
