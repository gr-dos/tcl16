# Provenance

Every input tcl16's M1 build uses, where it came from, how it is checked,
and every change made to it (docs/DESIGN.md, section 2.1, step 1).

## Tiny Tcl 6.8.1 (vendored)

| | |
|---|---|
| Project | Tiny Tcl, "Tcl for Embedded Applications", <https://tinytcl.sourceforge.net/>; SourceForge project `tinytcl` |
| Authors | Karl Lehenbauer and others (procplace.com, with TTC/Acterna); derived from Tcl 6.7 (University of California) and TclX 6 (Karl Lehenbauer and Mark Diekhans) |
| Release | TinyTcl 6.8.1, 11 September 2001 (SourceForge file release dated 2001-09-11 17:37 UTC) |
| Archive | `tinytcl681.zip`, 269952 bytes |
| URL | <https://sourceforge.net/projects/tinytcl/files/tinytcl/TinyTcl%206.8.1/tinytcl681.zip/download> |
| SHA-256 | `6a4d24de076a84285218cd398ab97a275650b127dcc00a3bf9ae06d92ca1c0f4` |
| Vendored as | `vendor/tinytcl-6.8.1/`: the archive's `tinytcl/` directory, all 72 files, byte for byte (CRLF line ends; `.gitattributes` keeps them) |
| Checked by | `vendor/tinytcl-6.8.1.sha256` (every file); `make check-vendor` checks the tree against it, and `make check-vendor-zip` also downloads the archive, checks its SHA-256 and compares every file (`scripts/check-vendor`) |

### Why 6.8.1, and the other copies

SourceForge has two file releases and a CVS repository:

- **6.8.0**, `tinytcl680.zip` (29 April 2001, SHA-256
  `160e22ffda80869a2c9d3336f4e24ec8f44419bc73b2be18b473f6050bdd129b`).
  It also holds a prebuilt Borland `TCL.EXE` and `CVS/` directories.
- **6.8.1**, `tinytcl681.zip` (11 September 2001), the latest release.
  Without CR and the RCS `$Id$` lines, it differs from 6.8.0 only in
  `dos.c` (`#define COMPILE_...` switches for some DOS commands), a
  comment in `tcldos.h`, `readme.txt`, `tinydocs/` and `tinytcl.c`. Its
  `tinytcl.c` is RCS revision 1.1.1.1, older than 6.8.0's 1.2, with the
  banner "Embedded Tcl 6.8-A". Of these files only `tcldos.h` is in
  tcl16's build, and its change is a comment.
- **CVS**, `https://sourceforge.net/code-snapshots/cvs/t/ti/tinytcl.zip`
  (read-only; the snapshot fetched for M1 had SHA-256
  `3fb4c2d18881577d0692f27d1efc5f20a4592318cc720636ce901870e0f50cec`,
  but SourceForge regenerates it, so it is not pinned). Its last commits
  are of 30 April 2001 (`dos.c` 1.2, `dosdefs.h`, `doc/CmdWrite.man`) and
  4 May 2001 (`LICENSE.txt` 1.1): nothing after 6.8.1 that tcl16 builds.

tcl16 vendors 6.8.1, the latest release, rather than fetching it,
because SourceForge's download URLs redirect to mirrors (DESIGN.md,
section 9, proposes `vendor/`).

### License

Neither archive has a license file. The source files carry their own
notices:

- Most files: "Copyright 1987-1992 Regents of the University of
  California. Permission to use, copy, modify, and distribute this
  software and its documentation for any purpose and without fee is
  hereby granted, provided that the above copyright notice appear in all
  copies..." (the Tcl 6 terms).
- `tclXdbg.c`, `tclXgen.c`, `tclExtend.h`, `tclExtdInt.h`, `readdir.c`:
  Karl Lehenbauer and Mark Diekhans, with the same permission.
- `regexp.c`: Henry Spencer's regexp, "Copyright (c) 1986 by University
  of Toronto", with his permission notice.
- `dos.c`, `borland.c`, `regexp.h`: no notice. (tcl16 does not build
  `dos.c` or `borland.c`.)

The CVS repository's `LICENSE.txt` (revision 1.1, 4 May 2001) is the
Tcl/Tk license, naming the Regents of the University of California, Sun
Microsystems, Scriptics, procplace.com, Acterna and others; SourceForge
lists the project as BSD-licensed. It is kept verbatim, extracted from
the RCS file's head revision, as `vendor/LICENSE.tinytcl`, since the
release archives omit it.

These terms permit tcl16 to modify and distribute the code with the
notices kept. Whether the chosen license for tcl16's own files is
compatible is still to be decided (DESIGN.md, section 9).

## Patches

`build/src` is `vendor/tinytcl-6.8.1/*.c` and `*.h` with CR removed, then
`patches/*.patch` applied in order with `patch -p1 -F0` (Makefile). Each
patch begins with its explanation; docs/M1-REPORT.md has the evidence.

| Patch | Change |
|---|---|
| `0001-watcom-build.patch` | Builds with Open Watcom: include names, Borland headers, C library declarations, `setenv`, `S_IFLNK`, prototypes for `read`/`getcwd` |
| `0002-ansi-varargs.patch` | `Tcl_AppendResult`, `Tcl_SetErrorCode`, `Tcl_VarEval` and `panic` defined with `<stdarg.h>` and declared with prototypes (the register calling convention broke every error message) |
| `0003-int16.patch` | 16-bit `int`: `end` and "no limit" sentinels, range checks in `Tcl_GetInt`, `incr` and `format` in `long` (`TclGetLong`), `source` of files of 32767 bytes or more |
| `0004-stack-guard.patch` | `Tcl_Eval` stops with a Tcl error when the stack is nearly gone (`TclStackExhausted`, in `src/stack.c`) |
| `0005-sizes.patch` | `ckalloc` checks for NULL; buffer growth and its comparisons in `long`, with a clean stop past `INT_MAX`; `format` width and precision |

All patches are tcl16's own work, under Tiny Tcl's terms as modifications
of it.

## tcl16's own files

`src/main.c` (the M1 driver), `src/stack.c` (the stack guard),
`src/probe.c` (the `m1probe` measurement command), `scripts/`, `tests/`
and `docs/` are written for tcl16. `tests/run-dos` is adapted from
gr-dos/perl16's `tests/run-dos`; `scripts/fetch-libc16` is libc16's
`consumers/fetch-libc16` with the 0.04 pin.

None of this work touches libc16. The libc16 experiment
(`scripts/try-libc16`) writes declaration-only shims inside
`build/libc16-try` to see past missing headers; they are not proposed for
libc16 (DESIGN.md, section 4).

## Toolchain and runtime

| Input | Pin |
|---|---|
| libc16 | release 0.04, `libc16-0.04.zip`, SHA-256 `f7c9af7daacfa236600b4d203a02f7c0ddedf55ce1b5120220d9aaf8a9d28e6e` (`scripts/fetch-libc16`), source revision `a37266e0aab307a049d4ae1e9148cdc814896cf7` |
| Open Watcom | open-watcom-v2 release `2026-10-01-Build`, `ow-snapshot.tar.xz`, SHA-256 `e6aa1b1e40ac8bbf97658d2c70fff8a4242d6ca4a1c60806f2baa5317083d4fe`, the snapshot libc16 0.04 names (its `tools/fetch-watcom`) |
| DOSBox-X | the host's package, for the tests and measurements only |
| gcc | the host's, with `-m32`, for `make host` only |

libc16 0.05 (`rename`, `unlink`, `fseek`, `ftell`) exists in the libc16
repository but was not released when M1 was done; see M1-REPORT.md,
section 6.
