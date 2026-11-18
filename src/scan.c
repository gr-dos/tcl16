/*
 * scan.c - the string scanner behind Tcl's "scan" command
 * (patches/0010-libc16.patch).
 *
 * Tiny Tcl's Tcl_ScanCmd hands its whole format to the C library's
 * sscanf, with up to 20 result pointers. Neither libc16 nor libkitchen16
 * has sscanf, so Tcl16 scans the string itself, and only for the formats
 * Tcl_ScanCmd passes: the conversions it accepts, with an "l" before
 * each integer one (ScanLongFormat, patches/0006-scan-long.patch).
 *
 * Tcl16_Sscanf(string, format, ...) follows C89 7.9.6.2 for those
 * formats:
 *
 * - white space in the format matches any amount of white space in the
 *   string, none included; any other character but "%" must match the
 *   next character of the string;
 * - "%%" matches "%";
 * - "%" may be followed by "*" (convert, but store nothing), then a
 *   field width, then "l", then the conversion:
 *   d, o, x    an optionally signed integer in base 10, 8 or 16 ("0x" or
 *              "0X" allowed before a hexadecimal one), stored in a long
 *              with "l", else in an int; beyond a long it wraps modulo
 *              2^32, as Open Watcom's sscanf did;
 *   s          a run of characters other than white space, stored with
 *              a terminating NUL;
 *   c          one character (or width characters), with no white space
 *              skipped and no NUL added;
 *   [...]      a run of characters in the set ("[^...]": not in it; a
 *              "]" first is a member; "a-z" is a range), with a NUL;
 *   e, f, g    a floating constant as strtod reads it, stored in a float,
 *              or in a double with "l";
 *   E, F, G    the same, always in a double: Tcl_ScanCmd gives "%E" and
 *              "%F" a double's room and reads them back as one;
 * - each conversion but c and [ first skips white space.
 *
 * It returns the number of fields stored, or EOF if the string ended
 * before the first conversion.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl16.h"

#ifdef TCL16_LIBC16
#include <kitchen16.h>
#endif

/*
 * The longest floating constant read, in characters: more than any
 * double needs, and a field width beyond it is cut to it.
 */

#define FLOAT_MAX 80

/*
 * Whether c is in the scan set from set (just after the "[") to end
 * (the closing "]").
 */

static int
InSet(int c, const char *set, const char *end)
{
    int negate = 0, in = 0;
    const char *p = set;

    if (*p == '^') {
	negate = 1;
	p++;
    }
    if (p < end && *p == ']') {
	in = (c == ']');
	p++;
    }
    for (; p < end && !in; p++) {
	if (p[1] == '-' && p + 2 < end) {
	    in = ((unsigned char) p[0] <= c && c <= (unsigned char) p[2]);
	    p += 2;
	} else {
	    in = (c == (unsigned char) *p);
	}
    }
    return in != negate;
}

/*
 * The value of digit c in base, or -1.
 */

static int
DigitValue(int c, int base)
{
    int v;

    if (c >= '0' && c <= '9') {
	v = c - '0';
    } else if (c >= 'a' && c <= 'z') {
	v = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
	v = c - 'A' + 10;
    } else {
	return -1;
    }
    return (v < base) ? v : -1;
}

int
Tcl16_Sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    const unsigned char *s = (const unsigned char *) str;
    const char *f = fmt;
    int stored = 0, converted = 0, inputFailure = 0;

    va_start(ap, fmt);
    while (*f != 0) {
	int suppress = 0, isLong = 0, width = 0, conv;

	if (isspace((unsigned char) *f)) {
	    while (isspace((unsigned char) *f)) {
		f++;
	    }
	    while (isspace(*s)) {
		s++;
	    }
	    continue;
	}
	if (*f != '%' || f[1] == '%') {
	    if (*f == '%') {
		f++;
	    }
	    if (*s != (unsigned char) *f) {
		break;
	    }
	    s++;
	    f++;
	    continue;
	}
	f++;
	if (*f == '*') {
	    suppress = 1;
	    f++;
	}
	while (isdigit((unsigned char) *f)) {
	    width = width * 10 + (*f++ - '0');
	}
	if (*f == 'l') {
	    isLong = 1;
	    f++;
	}
	conv = (unsigned char) *f;
	if (conv == 0) {
	    break;
	}
	f++;
	if (conv != 'c' && conv != '[') {
	    while (isspace(*s)) {
		s++;
	    }
	}
	if (*s == 0) {
	    inputFailure = 1;
	    break;
	}

	switch (conv) {
	case 'd': case 'o': case 'x': {
	    int base = (conv == 'd') ? 10 : (conv == 'o') ? 8 : 16;
	    int neg = 0, n = 0, max = width ? width : 32767;
	    unsigned long v = 0;
	    const unsigned char *start;

	    if (n < max && (*s == '+' || *s == '-')) {
		neg = (*s == '-');
		s++;
		n++;
	    }
	    if (base == 16 && n + 2 <= max && s[0] == '0'
		    && (s[1] == 'x' || s[1] == 'X')
		    && DigitValue(s[2], 16) >= 0) {
		s += 2;
		n += 2;
	    }
	    start = s;
	    while (n < max && DigitValue(*s, base) >= 0) {
		v = v * base + DigitValue(*s, base);
		s++;
		n++;
	    }
	    if (s == start) {
		goto done;		/* Matching failure. */
	    }
	    if (neg) {
		v = 0UL - v;
	    }
	    if (!suppress) {
		if (isLong) {
		    *va_arg(ap, long *) = (long) v;
		} else {
		    *va_arg(ap, int *) = (int) v;
		}
		stored++;
	    }
	    break;
	}

	case 's': {
	    char *dst = suppress ? NULL : va_arg(ap, char *);
	    int n = 0, max = width ? width : 32767;

	    while (n < max && *s != 0 && !isspace(*s)) {
		if (dst != NULL) {
		    *dst++ = (char) *s;
		}
		s++;
		n++;
	    }
	    if (dst != NULL) {
		*dst = 0;
		stored++;
	    }
	    break;
	}

	case 'c': {
	    char *dst = suppress ? NULL : va_arg(ap, char *);
	    int n = 0, max = width ? width : 1;

	    while (n < max && *s != 0) {
		if (dst != NULL) {
		    *dst++ = (char) *s;
		}
		s++;
		n++;
	    }
	    if (n < max) {
		goto done;
	    }
	    if (dst != NULL) {
		stored++;
	    }
	    break;
	}

	case '[': {
	    const char *set = f, *end;
	    char *dst;
	    int n = 0, max = width ? width : 32767;

	    end = set;
	    if (*end == '^') {
		end++;
	    }
	    if (*end == ']') {
		end++;
	    }
	    while (*end != 0 && *end != ']') {
		end++;
	    }
	    f = (*end == ']') ? end + 1 : end;
	    dst = suppress ? NULL : va_arg(ap, char *);
	    while (n < max && *s != 0 && InSet(*s, set, end)) {
		if (dst != NULL) {
		    *dst++ = (char) *s;
		}
		s++;
		n++;
	    }
	    if (n == 0) {
		goto done;
	    }
	    if (dst != NULL) {
		*dst = 0;
		stored++;
	    }
	    break;
	}

	case 'e': case 'f': case 'g': case 'E': case 'F': case 'G': {
	    char buf[FLOAT_MAX + 1], *end;
	    int n = 0, max = (width && width < FLOAT_MAX) ? width : FLOAT_MAX;
	    double d;

	    while (n < max && s[n] != 0 && !isspace(s[n])) {
		buf[n] = (char) s[n];
		n++;
	    }
	    buf[n] = 0;
	    d = strtod(buf, &end);
	    if (end == buf) {
		goto done;
	    }
	    s += end - buf;
	    if (!suppress) {
		if (isLong || isupper(conv)) {
		    *va_arg(ap, double *) = d;
		} else {
		    *va_arg(ap, float *) = (float) d;
		}
		stored++;
	    }
	    break;
	}

	default:
	    goto done;
	}
	converted++;
    }
  done:
    va_end(ap);
    return (converted == 0 && inputFailure) ? EOF : stored;
}
