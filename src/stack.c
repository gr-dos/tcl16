/*
 * stack.c - tcl16's stack guard for Tcl_Eval (patches/0004-stack-guard.patch;
 * docs/M1-REPORT.md, section 7.3).
 *
 * TclStackExhausted() is true when less than TCL16_STACK_MARGIN bytes of
 * stack are left. The margin must cover what runs between two Tcl_Eval
 * levels without Open Watcom's stack check, chiefly the C library's
 * sprintf and strtod; it is measured by tests/dos/deep.tcl and
 * scripts/measure.
 *
 * The development build asks Open Watcom's runtime (stackavail, with
 * TCL16_STACKAVAIL defined). Elsewhere (the host build, the libc16
 * experiment) there is no guard yet: libc16 has no stackavail, and the
 * host's stack is large.
 */

#include "tclInt.h"

#ifndef TCL16_STACK_MARGIN
#define TCL16_STACK_MARGIN 3072
#endif

#ifdef TCL16_STACKAVAIL
#include <malloc.h>

int
TclStackExhausted(void)
{
    return stackavail() < TCL16_STACK_MARGIN;
}
#else
int
TclStackExhausted(void)
{
    return 0;
}
#endif
