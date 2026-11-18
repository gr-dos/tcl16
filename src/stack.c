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
 * TCL.EXE, linked with libc16 (TCL16_LIBC16 defined), measures the stack
 * itself: libc16's startup puts the stack at the end of DGROUP, after
 * BSS, with SS = DGROUP, so what is left is SP less the end of BSS
 * (wlink's _end; libc16's docs/spec/startup.md). The measurement builds,
 * linked with Open Watcom's runtime, ask it (stackavail, with
 * TCL16_STACKAVAIL defined). The host build has no guard: its stack is
 * large.
 */

#include "tclInt.h"

#ifndef TCL16_STACK_MARGIN
#define TCL16_STACK_MARGIN 3072
#endif

#if defined(TCL16_LIBC16)
#include <dos.h>

extern char __cdecl end[];	/* The end of BSS: the stack's bottom. */

static unsigned GetSP(void);
#pragma aux GetSP = "mov ax,sp" value [ax] modify exact [ax];

int
TclStackExhausted(void)
{
    char __far *bottom = end;

    return GetSP() - FP_OFF(bottom) < TCL16_STACK_MARGIN;
}
#elif defined(TCL16_STACKAVAIL)
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
