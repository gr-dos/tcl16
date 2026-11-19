/*
 * dosinfo.c - the "dos" command: what DOS, and which kernel, TCL.EXE runs
 * on (docs/DESIGN.md, section 8.2; docs/COMPATIBILITY.md, "DOS and
 * GR-DOS").
 *
 *   dos version         the MS-DOS version DOS reports (INT 21h 30h), as
 *                       n.nn: 4.00 on MS-DOS 4.00 and on GR-DOS
 *   dos kernel          GR-DOS, if the kernel answers GR-DOS's
 *                       identification call; otherwise unknown
 *   dos kernelversion   GR-DOS's release version, n.nn (0.04); an empty
 *                       string when the kernel is not GR-DOS
 *   dos memory          the largest free block of conventional memory, in
 *                       bytes: what a program exec runs can have
 *                       (INT 21h 48h with BX = FFFFh)
 *   dos drives          the drives DOS knows, as a list: A: C: ...
 *
 * One command with options, as "file", "info" and "string" are, keeps
 * the names in one place and out of the way of scripts' own procs. A
 * name such as dos::version would suggest namespaces, which Tcl 6.7 does
 * not have. The option must be given in full: "kernel" is a prefix of
 * "kernelversion".
 *
 * GR-DOS's identification call is INT 21h AX=3347h, BX=4752h ("GR"),
 * with a nonce in DX (gr-dos/gr-dos#46; gr-dos's docs/grver.md). GR-DOS
 * answers AX=4752h, BH the major version, BL the minor (0 to 99), CL the
 * API revision (at least 1), CH reserved, and DX the nonce complemented.
 * Any other DOS takes it for an unknown subfunction of function 33h:
 * MS-DOS returns AL=FFh, DR DOS sets the carry flag and AX=0001h.
 * Like gr-dos's GRVER, this makes the call twice, with the same two
 * nonces, and takes the kernel for GR-DOS only if both answers pass every
 * check and agree. Anything else, including an answer with GR-DOS's
 * signature that fails a check, is "unknown": never an error.
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "tclInt.h"
#include "tcl16.h"

#define GRDOS_ID_AX	0x3347u	/* Function 33h, subfunction 47h ("G"). */
#define GRDOS_ID_SIG	0x4752u	/* "GR": the request and the answer. */

/*
 * AskGrDos: one identification call, with the nonce given. 1 and the
 * version if the answer is GR-DOS's, 0 otherwise.
 */

static int
AskGrDos(unsigned nonce, unsigned *major, unsigned *minor)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.ax = GRDOS_ID_AX;
    r.x.bx = GRDOS_ID_SIG;
    r.x.cx = 0;
    r.x.dx = nonce;
    intdos(&r, &r);
    if (r.x.ax != GRDOS_ID_SIG || r.x.dx != (unsigned) ~nonce
	    || r.h.cl == 0 || r.h.bl > 99) {
	return 0;
    }
    *major = r.h.bh;
    *minor = r.h.bl;
    return 1;
}

/*
 * GrDosVersion: 1 and GR-DOS's release version if the kernel is GR-DOS,
 * 0 otherwise. The second nonce catches a handler that answers every
 * call the same way.
 */

static int
GrDosVersion(unsigned *major, unsigned *minor)
{
    unsigned major2, minor2;

    if (!AskGrDos(0x5A3Cu, major, minor)
	    || !AskGrDos(0xC3A5u, &major2, &minor2)) {
	return 0;
    }
    return *major == major2 && *minor == minor2;
}

/*
 * DriveExists: whether DOS knows drive LETTER, by asking it to parse
 * "X:" into an FCB (INT 21h 29h), which fails with AL=FFh for a drive
 * that does not exist. It does not touch the drive, so an empty floppy
 * drive is not an error, and nothing asks to "Abort, Retry, Fail".
 */

static int
DriveExists(int letter)
{
    union REGS r;
    struct SREGS s;
    char name[3];
    char fcb[37];
    char *p;

    name[0] = (char) letter;
    name[1] = ':';
    name[2] = '\0';
    memset(&r, 0, sizeof(r));
    segread(&s);
    r.x.ax = 0x2900;		/* AL = 0: parse from the first byte. */
    p = name;
    s.ds = FP_SEG(p);
    r.x.si = FP_OFF(p);
    p = fcb;
    s.es = FP_SEG(p);
    r.x.di = FP_OFF(p);
    intdosx(&r, &r, &s);
    return r.h.al != 0xFF;
}

static int
WrongArgs(Tcl_Interp *interp, char *cmd, char *option)
{
    Tcl_AppendResult(interp, "wrong # args: should be \"", cmd, " ", option,
	    "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 * Tcl16_DosCmd: the "dos" command.
 */

int
Tcl16_DosCmd(ClientData dummy, Tcl_Interp *interp, int argc, char **argv)
{
    union REGS r;
    unsigned major, minor;
    char *option;

    (void) dummy;
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option\"", (char *) NULL);
	return TCL_ERROR;
    }
    option = argv[1];
    if (strcmp(option, "version") == 0) {
	if (argc != 2) {
	    return WrongArgs(interp, argv[0], option);
	}
	memset(&r, 0, sizeof(r));
	r.h.ah = 0x30;
	intdos(&r, &r);
	sprintf(interp->result, "%u.%02u", (unsigned) r.h.al,
		(unsigned) r.h.ah);
    } else if (strcmp(option, "kernel") == 0) {
	if (argc != 2) {
	    return WrongArgs(interp, argv[0], option);
	}
	interp->result = GrDosVersion(&major, &minor) ? "GR-DOS" : "unknown";
    } else if (strcmp(option, "kernelversion") == 0) {
	if (argc != 2) {
	    return WrongArgs(interp, argv[0], option);
	}
	if (GrDosVersion(&major, &minor)) {
	    sprintf(interp->result, "%u.%02u", major, minor);
	}
    } else if (strcmp(option, "memory") == 0) {
	if (argc != 2) {
	    return WrongArgs(interp, argv[0], option);
	}
	memset(&r, 0, sizeof(r));
	r.h.ah = 0x48;
	r.x.bx = 0xFFFF;
	intdos(&r, &r);
	if (!r.x.cflag) {
	    /*
	     * DOS gave a megabyte, which it cannot have: give it back.
	     */
	    union REGS f;
	    struct SREGS s;

	    memset(&f, 0, sizeof(f));
	    segread(&s);
	    f.h.ah = 0x49;
	    s.es = r.x.ax;
	    intdosx(&f, &f, &s);
	    r.x.bx = 0xFFFF;
	}
	sprintf(interp->result, "%lu", (unsigned long) r.x.bx * 16);
    } else if (strcmp(option, "drives") == 0) {
	int letter;
	char drive[3];

	if (argc != 2) {
	    return WrongArgs(interp, argv[0], option);
	}
	drive[1] = ':';
	drive[2] = '\0';
	for (letter = 'A'; letter <= 'Z'; letter++) {
	    if (DriveExists(letter)) {
		drive[0] = (char) letter;
		Tcl_AppendElement(interp, drive, 0);
	    }
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", option,
		"\": should be drives, kernel, kernelversion, memory, or version",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
