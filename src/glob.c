/*
 * glob.c - the "glob" command, for DOS (docs/DESIGN.md, sections 7 and
 * 8.1; docs/COMPATIBILITY.md, "Files and paths"). It replaces Tiny Tcl's
 * tcldosgl.c, which is not built.
 *
 *   glob ?-nocomplain? PATTERN ?PATTERN ...?
 *
 * As in Tcl 6.7, each pattern is a file name whose components may hold
 * "*", "?", "[chars]" and "{a,b,...}", and glob returns the names that
 * match, as a list, in the order DOS gives them, or an error if none
 * match and -nocomplain is not given. A component with no wildcard is
 * taken as it is, whether or not it exists; "{a,b}" makes both names.
 * Names starting with "." match only a pattern that starts with ".".
 *
 * On DOS:
 *
 * - "/" and "\" both separate components, and each separator is kept as
 *   the pattern has it; a drive, "C:", may start the pattern. "\" is not
 *   an escape: DOS names cannot hold "*", "?", "[" or "]".
 * - Matching ignores case, as DOS does: "*.tcl" matches "MAIN.TCL". The
 *   names found come back as DOS stores them, in upper case; the parts of
 *   the pattern with no wildcard keep the case they were written in.
 * - Hidden and system files are not listed, as DOS's DIR does not list
 *   them (src/dosdir.c).
 * - There is no "~": DOS has no home directories.
 */

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "tclInt.h"
#include "tcl16.h"
#include "dosdir.h"

/*
 * Whether name matches pattern (up to patEnd), ignoring case.
 */

static int
MatchNoCase(const char *name, const char *pat, const char *patEnd)
{
    while (pat < patEnd) {
	int c = toupper((unsigned char) *pat);

	if (c == '*') {
	    pat++;
	    if (pat == patEnd) {
		return 1;
	    }
	    for (;; name++) {
		if (MatchNoCase(name, pat, patEnd)) {
		    return 1;
		}
		if (*name == 0) {
		    return 0;
		}
	    }
	}
	if (*name == 0) {
	    return 0;
	}
	if (c == '?') {
	    /* Any one character. */
	} else if (c == '[') {
	    int n = toupper((unsigned char) *name), found = 0;

	    pat++;
	    while (pat < patEnd && *pat != ']') {
		int lo = toupper((unsigned char) *pat), hi = lo;

		if (pat + 2 < patEnd && pat[1] == '-' && pat[2] != ']') {
		    hi = toupper((unsigned char) pat[2]);
		    pat += 2;
		    if (lo > hi) {
			int t = lo;
			lo = hi;
			hi = t;
		    }
		}
		if (lo <= n && n <= hi) {
		    found = 1;
		}
		pat++;
	    }
	    if (!found) {
		return 0;
	    }
	    if (pat == patEnd) {
		return 1;		/* An unclosed "[": as Tcl 6.7. */
	    }
	} else if (c != toupper((unsigned char) *name)) {
	    return 0;
	}
	pat++;
	name++;
    }
    return *name == 0;
}

/*
 * Concatenates a, the first bLength bytes of b, and c (NUL-terminated),
 * into a string made with ckalloc.
 */

static char *
Concat(const char *a, const char *b, int bLength, const char *c)
{
    int aLength = strlen(a);
    char *s = ckalloc((unsigned) (aLength + bLength + strlen(c) + 1));

    memcpy(s, a, aLength);
    memcpy(s + aLength, b, bLength);
    strcpy(s + aLength + bLength, c);
    return s;
}

/*
 * DoGlob: adds to interp's result the names that match rem, a pattern
 * relative to dir. dir has no wildcards and is "", or ends with a
 * separator or a drive's ":", so that a name in it is dir followed by
 * the name.
 */

static int
DoGlob(Tcl_Interp *interp, char *dir, char *rem)
{
    char *p, *openBrace = NULL, *closeBrace = NULL;
    char sep[2];
    int special = 0, result = TCL_OK;

    /*
     * The first component of rem, up to p, and what it holds.
     */

    for (p = rem; *p != 0 && !TCL16_IS_SEP(*p); p++) {
	if (*p == '{' && openBrace == NULL) {
	    openBrace = p;
	}
	if (*p == '}' && closeBrace == NULL) {
	    closeBrace = p;
	}
	if (*p == '*' || *p == '?' || *p == '[') {
	    special = 1;
	}
    }
    sep[0] = *p;
    sep[1] = 0;

    /*
     * "{a,b}": glob each alternative, with the rest of rem.
     */

    if (openBrace != NULL) {
	char *element = openBrace + 1, *end;

	if (closeBrace == NULL || closeBrace < openBrace) {
	    Tcl_ResetResult(interp);
	    interp->result = "unmatched open-brace in file name";
	    return TCL_ERROR;
	}
	for (;;) {
	    char *before, *alt;

	    for (end = element; end < closeBrace && *end != ','; end++) {
		/* Find the end of this alternative. */
	    }
	    before = Concat("", rem, (int) (openBrace - rem), "");
	    alt = Concat(before, element, (int) (end - element),
		    closeBrace + 1);
	    ckfree(before);
	    result = DoGlob(interp, dir, alt);
	    ckfree(alt);
	    if (result != TCL_OK || end == closeBrace) {
		return result;
	    }
	    element = end + 1;
	}
    }

    /*
     * No wildcard: the component as it is.
     */

    if (!special) {
	if (*p == 0) {
	    char *name = Concat(dir, rem, (int) (p - rem), "");

	    Tcl_AppendElement(interp, name, 0);
	    ckfree(name);
	} else {
	    char *newDir = Concat(dir, rem, (int) (p - rem), sep);

	    result = DoGlob(interp, newDir, p + 1);
	    ckfree(newDir);
	}
	return result;
    }

    /*
     * Wildcards: the entries of dir that match.
     */

    {
	DIR *d = opendir((*dir == 0) ? "." : dir);
	struct dirent *e;

	if (d == NULL) {
	    if (errno == ENOMEM) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "couldn't read directory \"", dir,
			"\": ", Tcl_UnixError(interp), (char *) NULL);
		return TCL_ERROR;
	    }
	    return TCL_OK;		/* No such directory: no names. */
	}
	while (result == TCL_OK && (e = readdir(d)) != NULL) {
	    if (e->d_name[0] == '.' && *rem != '.') {
		continue;
	    }
	    if (!MatchNoCase(e->d_name, rem, p)) {
		continue;
	    }
	    if (*p == 0) {
		char *name = Concat(dir, e->d_name, strlen(e->d_name), "");

		Tcl_AppendElement(interp, name, 0);
		ckfree(name);
	    } else if (e->d_attr & 0x10) {	/* A directory. */
		char *newDir = Concat(dir, e->d_name, strlen(e->d_name), sep);

		result = DoGlob(interp, newDir, p + 1);
		ckfree(newDir);
	    }
	}
	closedir(d);
    }
    return result;
}

/*
 * Tcl_TildeSubst: Tcl's "~" substitution, which tcldosaz.c calls for
 * file names. DOS has no home directories: names are kept as they are.
 */

char *
Tcl_TildeSubst(Tcl_Interp *interp, char *name)
{
    return name;
}

	/* ARGSUSED */
int
Tcl_GlobCmd(dummy, interp, argc, argv)
    ClientData dummy;
    Tcl_Interp *interp;
    int argc;
    char **argv;
{
    int i, noComplain = 0;

    if (argc >= 2 && strcmp(argv[1], "-nocomplain") == 0) {
	noComplain = 1;
    }
    if (argc < 2 + noComplain) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" ?-nocomplain? name ?name ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    for (i = 1 + noComplain; i < argc; i++) {
	char *pattern = argv[i], *dir;
	int prefix = Tcl16_PathPrefix(pattern), result;

	/*
	 * The drive and the root, if any, are the starting directory.
	 */

	if (TCL16_IS_SEP(pattern[prefix])) {
	    prefix++;
	}
	dir = Concat("", pattern, prefix, "");
	result = DoGlob(interp, dir, pattern + prefix);
	ckfree(dir);
	if (result != TCL_OK) {
	    return result;
	}
    }
    if (*interp->result == 0 && !noComplain) {
	char *sep = "";

	Tcl_AppendResult(interp, "no files matched glob pattern",
		(argc == 2) ? " \"" : "s \"", (char *) NULL);
	for (i = 1; i < argc; i++) {
	    Tcl_AppendResult(interp, sep, argv[i], (char *) NULL);
	    sep = " ";
	}
	Tcl_AppendResult(interp, "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
