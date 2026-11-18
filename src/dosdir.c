/*
 * dosdir.c - directory enumeration for Tiny Tcl's glob and readdir
 * (dosdir.h), over the C library's _dos_findfirst and _dos_findnext
 * (INT 21h 4Eh and 4Fh).
 *
 * Tcl16_OpenDir(path) starts a search of "path\*.*" for files and
 * directories; hidden and system files are left out, as DOS's DIR leaves
 * them out. Each search has its own find_t (the DOS search record), so
 * that searches can nest, as glob's do. The names come back as DOS
 * stores them, in upper case, "." and ".." included where DOS gives
 * them (not in a root directory).
 *
 * Tcl16_OpenDir returns NULL with errno set when the directory cannot be
 * searched: ENOENT for a missing path, ENOMEM when no memory is left.
 * An empty directory (only a root can be) gives no entries.
 */

#include <dos.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "dosdir.h"

/*
 * The longest path DOS takes, in bytes with the NUL: 128.
 */

#define DOS_PATH_MAX 128

struct Tcl16Dir {
    struct find_t find;		/* The DOS search record. */
    int state;			/* NEXT, FIRST (find holds an entry not yet
				 * returned) or DONE. */
    struct dirent entry;	/* What Tcl16_ReadDir returns. */
};

#define STATE_NEXT	0
#define STATE_FIRST	1
#define STATE_DONE	2

DIR *
Tcl16_OpenDir(const char *path)
{
    char pattern[DOS_PATH_MAX];
    size_t len = strlen(path);
    DIR *dir;
    unsigned err;

    if (len + sizeof("\\*.*") > sizeof(pattern)) {
	errno = ENOENT;
	return NULL;
    }
    memcpy(pattern, path, len);
    if (len == 1 && path[0] == '.') {
	len = 0;			/* "." is the current directory. */
    } else if (len > 0 && path[len - 1] != '\\' && path[len - 1] != '/'
	    && path[len - 1] != ':') {
	pattern[len++] = '\\';
    }
    strcpy(pattern + len, "*.*");

    dir = (DIR *) malloc(sizeof(DIR));
    if (dir == NULL) {
	errno = ENOMEM;
	return NULL;
    }
    err = _dos_findfirst(pattern, _A_SUBDIR, &dir->find);
    if (err == 0) {
	dir->state = STATE_FIRST;
    } else if (err == 18) {		/* No more files: an empty root. */
	dir->state = STATE_DONE;
    } else {
	free(dir);
	errno = ENOENT;
	return NULL;
    }
    return dir;
}

struct dirent *
Tcl16_ReadDir(DIR *dir)
{
    if (dir->state == STATE_NEXT) {
	if (_dos_findnext(&dir->find) != 0) {
	    dir->state = STATE_DONE;
	}
    }
    if (dir->state == STATE_DONE) {
	return NULL;
    }
    dir->state = STATE_NEXT;
    strcpy(dir->entry.d_name, dir->find.name);
    dir->entry.d_attr = (unsigned char) dir->find.attrib;
    return &dir->entry;
}

int
Tcl16_CloseDir(DIR *dir)
{
    free(dir);
    return 0;
}
