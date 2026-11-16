/*
 * freemem.c - print the largest block DOS can allocate, in bytes
 * (INT 21h AH=48h, BX=FFFFh), for scripts/measure: run before TCL.EXE,
 * it is the conventional memory a program can have.
 */
#include <dos.h>
#include <stdio.h>

int
main(void)
{
    union REGS r;

    r.h.ah = 0x48;
    r.x.bx = 0xFFFF;
    intdos(&r, &r);
    printf("freemem %lu\n", (unsigned long) r.x.bx * 16UL);
    return 0;
}
