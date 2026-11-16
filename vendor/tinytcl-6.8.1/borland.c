/*
 * Stuff specific to the Borland C compiler.
 *
 * $Id: borland.c,v 1.1.1.1 2001/04/20 15:03:05 karl Exp $
 *
 */

/* _stklen defines how big the runtime stack is.  It must be set at
 * compile time as an assignment.
 */

extern unsigned _stklen = 65400U;
