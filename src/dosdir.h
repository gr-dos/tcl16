/*
 * dosdir.h - directory enumeration for Tiny Tcl's glob and readdir
 * (dosdir.c), over the C library's _dos_findfirst and _dos_findnext.
 *
 * libc16 has no <dirent.h>; tcldos.h includes this header in its place
 * (patches/0010-libc16.patch). The names upstream calls, opendir,
 * readdir and closedir, are macros for Tcl16's own functions, so that
 * they never collide with a C library's.
 */

#ifndef TCL16_DOSDIR_H
#define TCL16_DOSDIR_H

typedef struct Tcl16Dir DIR;

/*
 * Tcl16_OpenDir lists files and directories, but not hidden or system
 * files (as DOS's DIR); Tcl16_OpenDirAttr lists what attr asks for, as
 * INT 21h 4Eh reads it: TCL16_ALL_ENTRIES for everything but the volume
 * label.
 */

#define TCL16_ALL_ENTRIES 0x16	/* _A_SUBDIR | _A_HIDDEN | _A_SYSTEM */

struct dirent {
    char d_name[13];		/* The name, as DOS stores it: 8.3, upper
				 * case. */
    unsigned char d_attr;	/* Its DOS attributes (_A_SUBDIR...). */
};

extern DIR *		Tcl16_OpenDir(const char *path);
extern DIR *		Tcl16_OpenDirAttr(const char *path, unsigned attr);
extern struct dirent *	Tcl16_ReadDir(DIR *dir);
extern int		Tcl16_CloseDir(DIR *dir);

#define opendir		Tcl16_OpenDir
#define readdir		Tcl16_ReadDir
#define closedir	Tcl16_CloseDir

#endif /* TCL16_DOSDIR_H */
