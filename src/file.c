/*
 * file.c - the "file" command, with DOS paths (docs/DESIGN.md, section
 * 8.1; docs/COMPATIBILITY.md, "Files and paths").
 *
 * Tcl16 handles here what depends on DOS path syntax, and what Tcl 6.7
 * did not have:
 *
 *   file dirname NAME      the name up to its last "/" or "\", which
 *   file tail NAME         separate directories on DOS; a drive "X:"
 *   file rootname NAME     before the path is kept by dirname, and is
 *   file extension NAME    never part of a tail, root or extension
 *   file delete ?-force? ?--? NAME ?NAME ...?
 *   file mkdir DIR ?DIR ...?
 *   file rename ?-force? ?--? SOURCE TARGET
 *
 *   file executable NAME   1 for a directory, or a file whose name ends
 *                          in .COM, .EXE or .BAT: what DOS can run (DOS
 *                          has no execute permission)
 *
 * delete, mkdir and rename are Tcl16 extensions, with the syntax Tcl 7.6
 * later gave them, as far as DOS permits (below). The other options
 * (atime, exists, isdirectory, isfile, lstat, mtime, owned, readable,
 * size, stat, type, writable) are Tiny Tcl's, which
 * patches/0011-dos-file.patch renames TclDosFileCmd.
 *
 * Options may be abbreviated, as in Tcl 6.7. Every abbreviation Tcl 6.7
 * took keeps its meaning, so "d" is still dirname and "m" mtime; the
 * new options need "de", "mk" and "ren".
 */

#include "tclInt.h"
#include "tcldos.h"		/* The C library's file headers, S_ISDIR. */
#include "tcl16.h"

extern int TclDosFileCmd _ANSI_ARGS_((ClientData dummy, Tcl_Interp *interp,
	int argc, char **argv));

/*
 *----------------------------------------------------------------------
 *
 * DOS path syntax.
 *
 *----------------------------------------------------------------------
 */

/*
 * Tcl16_PathPrefix: the length of a drive "X:" at the start of name: 2,
 * or 0 if there is none.
 */

int
Tcl16_PathPrefix(const char *name)
{
    if (((name[0] >= 'A' && name[0] <= 'Z')
	    || (name[0] >= 'a' && name[0] <= 'z')) && name[1] == ':') {
	return 2;
    }
    return 0;
}

/*
 * Tcl16_PathTail: where the last component of name starts: after its
 * last "/" or "\", or after its drive.
 */

char *
Tcl16_PathTail(char *name)
{
    char *p, *tail = name + Tcl16_PathPrefix(name);

    for (p = tail; *p != 0; p++) {
	if (TCL16_IS_SEP(*p)) {
	    tail = p + 1;
	}
    }
    return tail;
}

/*
 * The "." that starts the extension of name, or NULL.
 */

static char *
Extension(char *name)
{
    char *dot = strrchr(name, '.');

    return (dot != NULL && dot >= Tcl16_PathTail(name)) ? dot : NULL;
}

/*
 * Sets the result to the first length bytes of name.
 */

static void
SetPrefixResult(Tcl_Interp *interp, char *name, int length)
{
    char saved = name[length];

    name[length] = 0;
    Tcl_SetResult(interp, name, TCL_VOLATILE);
    name[length] = saved;
}

static void
DirName(Tcl_Interp *interp, char *name)
{
    int prefix = Tcl16_PathPrefix(name);
    char *tail = Tcl16_PathTail(name);

    if (tail == name + prefix) {
	/* No separator: "C:NAME" is in "C:", "NAME" in ".". */
	if (prefix == 0) {
	    interp->result = ".";
	} else {
	    SetPrefixResult(interp, name, prefix);
	}
    } else if (tail == name + prefix + 1) {
	/* In the root: "\", "C:\". */
	SetPrefixResult(interp, name, prefix + 1);
    } else {
	SetPrefixResult(interp, name, (int) (tail - name) - 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * file delete.
 *
 *----------------------------------------------------------------------
 */

/*
 * Joins dir and name with a "\" (none after a separator or a drive) into
 * a string made with ckalloc.
 */

static char *
JoinPath(const char *dir, const char *name)
{
    int len = strlen(dir);
    char *path = ckalloc((unsigned) (len + strlen(name) + 2));

    strcpy(path, dir);
    if (len > 0 && !TCL16_IS_SEP(dir[len - 1]) && dir[len - 1] != ':') {
	path[len++] = '\\';
    }
    strcpy(path + len, name);
    return path;
}

/*
 * Whether name is a directory: stat succeeds and says so (stat reports
 * a root as one).
 */

static int
IsDirectory(const char *name)
{
    struct stat st;

    return stat(name, &st) == 0 && S_ISDIR(st.st_mode);
}

/*
 * Whether the directory dir has entries other than "." and "..",
 * hidden and system files included. -1 with errno if it cannot be read.
 */

static int
HasEntries(const char *dir)
{
    DIR *d = Tcl16_OpenDirAttr(dir, TCL16_ALL_ENTRIES);
    struct dirent *e;
    int found = 0;

    if (d == NULL) {
	return -1;
    }
    while (!found && (e = readdir(d)) != NULL) {
	found = strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0;
    }
    closedir(d);
    return found;
}

/*
 * Deletes a file; with force, a read-only one too. 0, or -1 with errno.
 */

static int
DeleteFile(const char *name, int force)
{
    unsigned attr;

    if (unlink(name) == 0) {
	return 0;
    }
    if (force && errno == EACCES && _dos_getfileattr(name, &attr) == 0
	    && (attr & _A_RDONLY)
	    && _dos_setfileattr(name, attr & ~_A_RDONLY) == 0) {
	return unlink(name);
    }
    return -1;
}

/*
 * Deletes the directory dir and, with force, everything in it. On an
 * error, leaves a message in interp naming orig, the name given to
 * "file delete", and returns TCL_ERROR.
 */

static int
DeleteDirectory(Tcl_Interp *interp, char *orig, char *dir, int force)
{
    int full, err;

    if (rmdir(dir) == 0) {
	return TCL_OK;
    }
    err = errno;
    full = HasEntries(dir);
    if (full == 1 && !force) {
	Tcl_AppendResult(interp, "error deleting \"", orig,
		"\": directory not empty", (char *) NULL);
	return TCL_ERROR;
    }
    if (full == 1) {
	DIR *d = Tcl16_OpenDirAttr(dir, TCL16_ALL_ENTRIES);
	struct dirent *e;
	int result = TCL_OK;

	if (d == NULL) {
	    err = errno;
	    goto error;
	}
	while (result == TCL_OK && (e = readdir(d)) != NULL) {
	    char *path;

	    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
		continue;
	    }
	    path = JoinPath(dir, e->d_name);
	    if (e->d_attr & _A_SUBDIR) {
		result = DeleteDirectory(interp, orig, path, force);
	    } else if (DeleteFile(path, force) != 0) {
		Tcl_AppendResult(interp, "error deleting \"", path, "\": ",
			Tcl_UnixError(interp), (char *) NULL);
		result = TCL_ERROR;
	    }
	    ckfree(path);
	}
	closedir(d);
	if (result != TCL_OK) {
	    return result;
	}
	if (rmdir(dir) == 0) {
	    return TCL_OK;
	}
	err = errno;
    }
  error:
    errno = err;
    Tcl_AppendResult(interp, "error deleting \"", orig, "\": ",
	    Tcl_UnixError(interp), (char *) NULL);
    return TCL_ERROR;
}

/*
 * Reads "?-force? ?--?" from argv[2] on; stores whether -force was given
 * and returns the index of the first other argument.
 */

static int
FileSwitches(int argc, char **argv, int *forcePtr)
{
    int i = 2;

    *forcePtr = 0;
    while (i < argc && argv[i][0] == '-') {
	if (strcmp(argv[i], "-force") == 0) {
	    *forcePtr = 1;
	} else if (strcmp(argv[i], "--") == 0) {
	    return i + 1;
	} else {
	    break;
	}
	i++;
    }
    return i;
}

static int
FileDelete(Tcl_Interp *interp, int argc, char **argv)
{
    int force, i = FileSwitches(argc, argv, &force);

    if (i >= argc) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" delete ?-force? ?--? name ?name ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    for (; i < argc; i++) {
	struct stat st;

	if (stat(argv[i], &st) != 0) {
	    if (errno == ENOENT) {
		continue;		/* Already gone: not an error. */
	    }
	    Tcl_AppendResult(interp, "error deleting \"", argv[i], "\": ",
		    Tcl_UnixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
	if (S_ISDIR(st.st_mode)) {
	    if (DeleteDirectory(interp, argv[i], argv[i], force) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (DeleteFile(argv[i], force) != 0) {
	    Tcl_AppendResult(interp, "error deleting \"", argv[i], "\": ",
		    Tcl_UnixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * file mkdir.
 *
 *----------------------------------------------------------------------
 */

/*
 * Makes dir, and any of its parents that are missing.
 */

static int
MakeDirectory(Tcl_Interp *interp, char *dir)
{
    char *p, saved;
    struct stat st;

    if (stat(dir, &st) == 0) {
	if (S_ISDIR(st.st_mode)) {
	    return TCL_OK;
	}
	Tcl_AppendResult(interp, "can't create directory \"", dir,
		"\": file already exists", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Each parent in turn, from the first component after the drive and
     * root.
     */

    p = dir + Tcl16_PathPrefix(dir);
    while (TCL16_IS_SEP(*p)) {
	p++;
    }
    for (;;) {
	while (*p != 0 && !TCL16_IS_SEP(*p)) {
	    p++;
	}
	saved = *p;
	*p = 0;
	if (stat(dir, &st) != 0) {
	    if (mkdir(dir) != 0) {
		*p = saved;
		Tcl_AppendResult(interp, "can't create directory \"", dir,
			"\": ", Tcl_UnixError(interp), (char *) NULL);
		return TCL_ERROR;
	    }
	} else if (!S_ISDIR(st.st_mode)) {
	    Tcl_AppendResult(interp, "can't create directory \"", dir,
		    "\": file already exists", (char *) NULL);
	    *p = saved;
	    return TCL_ERROR;
	}
	*p = saved;
	while (TCL16_IS_SEP(*p)) {
	    p++;
	}
	if (*p == 0) {
	    return TCL_OK;
	}
    }
}

static int
FileMkdir(Tcl_Interp *interp, int argc, char **argv)
{
    int i;

    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" mkdir name ?name ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    for (i = 2; i < argc; i++) {
	if (MakeDirectory(interp, argv[i]) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * file rename.
 *
 *----------------------------------------------------------------------
 */

static int
FileRename(Tcl_Interp *interp, int argc, char **argv)
{
    int force, i = FileSwitches(argc, argv, &force), result = TCL_OK;
    char *source, *target;
    struct stat st;

    if (argc - i != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" rename ?-force? ?--? source target\"", (char *) NULL);
	return TCL_ERROR;
    }
    source = argv[i];
    if (stat(source, &st) != 0) {
	Tcl_AppendResult(interp, "error renaming \"", source, "\": ",
		Tcl_UnixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Into an existing directory: under the source's own name.
     */

    if (IsDirectory(argv[i + 1])) {
	target = JoinPath(argv[i + 1], Tcl16_PathTail(source));
    } else {
	target = JoinPath("", argv[i + 1]);
    }

    if (stat(target, &st) == 0) {
	if (!force) {
	    Tcl_AppendResult(interp, "error renaming \"", source, "\" to \"",
		    target, "\": file already exists", (char *) NULL);
	    result = TCL_ERROR;
	} else if (S_ISDIR(st.st_mode) ? (rmdir(target) != 0)
		: (DeleteFile(target, 1) != 0)) {
	    Tcl_AppendResult(interp, "error renaming \"", source, "\" to \"",
		    target, "\": ", Tcl_UnixError(interp), (char *) NULL);
	    result = TCL_ERROR;
	}
    }
    if (result == TCL_OK && rename(source, target) != 0) {
	Tcl_AppendResult(interp, "error renaming \"", source, "\" to \"",
		target, "\": ", Tcl_UnixError(interp), (char *) NULL);
	result = TCL_ERROR;
    }
    ckfree(target);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FileCmd --
 *
 *	The "file" command. Its options, with the fewest letters that
 *	select each; an abbreviation shorter than that is a bad option.
 *	The first option in the table that an abbreviation selects wins:
 *	"d" is dirname, as in Tcl 6.7.
 *
 *----------------------------------------------------------------------
 */

#define OWN_DIRNAME	1
#define OWN_TAIL	2
#define OWN_ROOTNAME	3
#define OWN_EXTENSION	4
#define OWN_DELETE	5
#define OWN_MKDIR	6
#define OWN_RENAME	7
#define OWN_EXECUTABLE	8

static struct {
    char *name;
    int min;			/* Letters needed to select it. */
    int own;			/* Handled here (OWN_*), or 0: Tiny Tcl's. */
} options[] = {
    {"atime",		1,	0},
    {"delete",		2,	OWN_DELETE},
    {"dirname",		1,	OWN_DIRNAME},
    {"executable",	3,	OWN_EXECUTABLE},
    {"exists",		3,	0},
    {"extension",	3,	OWN_EXTENSION},
    {"isdirectory",	3,	0},
    {"isfile",		3,	0},
    {"lstat",		1,	0},
    {"mkdir",		2,	OWN_MKDIR},
    {"mtime",		1,	0},
    {"owned",		1,	0},
    {"readable",	5,	0},
    {"rename",		3,	OWN_RENAME},
    {"rootname",	2,	OWN_ROOTNAME},
    {"size",		2,	0},
    {"stat",		2,	0},
    {"tail",		2,	OWN_TAIL},
    {"type",		2,	0},
    {"writable",	1,	0},
    {NULL,		0,	0}
};

	/* ARGSUSED */
int
Tcl_FileCmd(dummy, interp, argc, argv)
    ClientData dummy;
    Tcl_Interp *interp;
    int argc;
    char **argv;
{
    int i, length;
    char *name, *p;

    if (argc >= 2) {
	length = strlen(argv[1]);
	for (i = 0; options[i].name != NULL; i++) {
	    if (length >= options[i].min
		    && strncmp(argv[1], options[i].name, length) == 0) {
		break;
	    }
	}
    }
    if (argc < 2 || (argc < 3 && (options[i].name == NULL
	    || options[i].own == 0))) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option name ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (options[i].name == NULL) {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be atime, delete, dirname, executable, exists, ",
		"extension, isdirectory, isfile, lstat, mkdir, mtime, owned, ",
		"readable, rename, rootname, size, stat, tail, type, ",
		"or writable", (char *) NULL);
	return TCL_ERROR;
    }

    switch (options[i].own) {
    case 0:
	return TclDosFileCmd(dummy, interp, argc, argv);
    case OWN_DELETE:
	return FileDelete(interp, argc, argv);
    case OWN_MKDIR:
	return FileMkdir(interp, argc, argv);
    case OWN_RENAME:
	return FileRename(interp, argc, argv);
    }

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], " ",
		options[i].name, " name\"", (char *) NULL);
	return TCL_ERROR;
    }
    name = argv[2];
    switch (options[i].own) {
    case OWN_DIRNAME:
	DirName(interp, name);
	break;
    case OWN_TAIL:
	Tcl_SetResult(interp, Tcl16_PathTail(name), TCL_VOLATILE);
	break;
    case OWN_ROOTNAME:
	p = Extension(name);
	if (p == NULL) {
	    Tcl_SetResult(interp, name, TCL_VOLATILE);
	} else {
	    SetPrefixResult(interp, name, (int) (p - name));
	}
	break;
    case OWN_EXTENSION:
	p = Extension(name);
	if (p != NULL) {
	    Tcl_SetResult(interp, p, TCL_VOLATILE);
	}
	break;
    case OWN_EXECUTABLE: {
	struct stat st;

	p = Extension(name);
	interp->result = (stat(name, &st) == 0 && (S_ISDIR(st.st_mode)
		|| (p != NULL && (stricmp(p, ".COM") == 0
		|| stricmp(p, ".EXE") == 0 || stricmp(p, ".BAT") == 0))))
		? "1" : "0";
	break;
    }
    }
    return TCL_OK;
}
