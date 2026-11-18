/*
 * probe.c - tcl16's M1 measurement command (docs/M1-REPORT.md, section 7).
 *
 *   m1probe dosfree    bytes in the largest block DOS can allocate
 *                      (INT 21h AH=48h with BX=FFFFh)
 *   m1probe stack      bytes of stack left (stack.c's Tcl16_StackLeft)
 *   m1probe ticks      the BIOS tick count at 0040:006C (18.2 a second)
 *
 * It exists only for the evaluation, and the names are not a proposal
 * for the DOS commands of M5 and M6 (docs/DESIGN.md, section 8.2).
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "tclInt.h"

int Tcl_InitProbe(Tcl_Interp *interp);
unsigned Tcl16_StackLeft(void);

static int
ProbeCmd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
    (void) clientData;
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " dosfree|stack|ticks\"", (char *) NULL);
        return TCL_ERROR;
    }
    if (strcmp(argv[1], "dosfree") == 0) {
        union REGS r;

        r.h.ah = 0x48;
        r.x.bx = 0xFFFF;
        intdos(&r, &r);
        sprintf(interp->result, "%lu", (unsigned long) r.x.bx * 16UL);
    } else if (strcmp(argv[1], "stack") == 0) {
        sprintf(interp->result, "%u", Tcl16_StackLeft());
    } else if (strcmp(argv[1], "ticks") == 0) {
        unsigned long __far *ticks = (unsigned long __far *) MK_FP(0x40, 0x6C);

        sprintf(interp->result, "%lu", *ticks);
    } else {
        Tcl_AppendResult(interp, "bad option \"", argv[1],
                "\": should be dosfree, stack or ticks", (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
Tcl_InitProbe(Tcl_Interp *interp)
{
    Tcl_CreateCommand(interp, "m1probe", ProbeCmd, (ClientData) NULL,
            (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
}
