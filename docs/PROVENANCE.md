# Provenance

Every input tcl16's builds use, where it came from, how it is checked,
and every change made to it (docs/DESIGN.md, section 2.1, step 1): M1's
candidate, Tiny Tcl 6.8.1, which is vendored, and M2's, the Tcl 6.7 DOS
port, which is fetched.

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
| `0006-scan-long.patch` | `scan` stores integer fields in a `long` (added in M2, when Tcl 6.7's own test suite found the 16-bit wrap in both candidates) |
| `0007-qsort-cdecl.patch` | `lsort`'s `qsort` comparator in the C library's calling convention: `__cdecl` for libc16 (`TCL16_CDECL`, with `TCL16_LIBC16`), found by `make libc16-try` with libc16 0.06 (M4) |
| `0008-assemble-size.patch` | `Tcl_AssembleCmd`'s buffer growth in `long` through `TclGrowSize`, as in 0005 (M4) |
| `0009-dos-source.patch` | Removes `Tcl_EvalFile` and `Tcl_SmallFootprintEvalFile` from `tcldosut.c`: Tcl16's own `src/source.c` reads scripts as DOS text (M4) |
| `0010-libc16.patch` | Links with libc16 and libkitchen16 (`TCL16_LIBC16`): `<kitchen16.h>` in `tclInt.h` and `panic.c`, `panic` exits with 255 rather than `abort`, `scan` calls Tcl16's `Tcl16_Sscanf` (`src/scan.c`), `tcldos.h` takes `opendir` from `src/dosdir.h`, and `tclEnv.c` reads the environment from `src/dosenv.c` and exports it as `TclEnviron` for `exec` (M5) |
| `0011-dos-file.patch` | Renames `tcldosaz.c`'s `Tcl_FileCmd` to `TclDosFileCmd`, which Tcl16's own `file` (`src/file.c`) calls for the options it does not handle (M5) |

All patches are tcl16's own work, under Tiny Tcl's terms as modifications
of it.

## Tcl 6.7 DOS port (fetched, not vendored)

| | |
|---|---|
| Project | Tcl 6.7 (John Ousterhout, University of California, Berkeley, 1993), ported to MS-DOS by Peter Sprenger, April 1993 |
| Archive | `tcl67.dos.port.zip`, 543793 bytes, in the Tcl project's SourceForge file area, `Tcl/6.7`, dated 2008-06-06 |
| URL | <https://sourceforge.net/projects/tcl/files/Tcl/6.7/tcl67.dos.port.zip/download> |
| SHA-256 | `ccab22130cc23280b2ff40c48a0edae5aec8080283c350d1b70af80d122880e8` (MD5 `c6c74b9b8498ceb2fff36a01af526e33`, as SourceForge lists it) |
| Contents | one file, `tcl67.dos.port/tcl67dos.zip` (570325 bytes, dated 1993-04-13; SHA-256 `e88e2e1024e17908ca0fe6d69b89661628d740835daff272d9eb3ce745f3abb5`), the port as Sprenger distributed it: 111 files under `TCL/` |
| Base | Tcl 6.7, `tcl6.7.tar.gz` from the same folder, 274513 bytes, SHA-256 `13ee4d194b906ddeb59f6d9dcefbf4c94ce431a00f10a57d877b1aa60f1449d0` (MD5 `dda83185cba00d95de8c29accc2c7ac5`) |
| Fetched by | `scripts/fetch-tcl67` (`make tcl67`), which checks all three SHA-256 values before unpacking, into `build/tcl67/` |

Older references name the port `tcl67dos.zip` on ftp.neosoft.com and
ftp.aud.alcatel.com. The SourceForge copy is the one still published.

### Not vendored, and why

tcl16 does not redistribute this archive:

- **`MSDOS.C` and `MSDOS.H`**, the port proper, say "MSDOS TCL port by
  Peter Sprenger" and carry no copyright or permission notice. Without
  one, nothing grants the right to redistribute them.
- **`TCLERRNO.H`** is a copy of Borland C++'s `errno.h`: "Copyright (c)
  1987, 1991 by Borland International, All Rights Reserved". tcl16's
  build does not use it (patch 0001 takes `<errno.h>` from the C
  library).
- **`UTIL/`** (`cat`, `wc`, `pwd`, `printenv`, `whoami`, sources and
  Borland executables) and **`DOC/`** (Tcl 6.2 documents in WordPerfect
  and text form, and Ousterhout's USENIX paper) came from John S.
  Martin's earlier Tcl 6.2 DOS port. The README says only that "There
  was no copyright message, so I think it is okay." `TCL.LIB` and
  `TCL.PRJ`/`TEST.PRJ` are Borland build products.
- The rest is Tcl 6.7 under the University of California's notice
  (above). It includes `TESTS/`, Tcl 6.7's own test suite, which tcl16
  runs from the fetched copy (`make test-suite-tcl67`, `make
  test-suite`).

So `make` fetches the archive, checks it and builds it in `build/`,
and the repository holds only tcl16's patches. The patches carry some
context lines from `msdos.c` and `msdos.h`, as any patch must. Had the
port been chosen, its own files would have needed the author's
permission, or a replacement written for tcl16. M3 chose Tiny Tcl
instead (ADR-001-tcl-implementation.md). The port stays fetched as
reference material, and its archive supplies Tcl 6.7's test suite.

### Normalisation and names

`build/tcl67/upstream/` is the port's C sources and headers with CR and
the DOS end-of-file byte (^Z, at the end of six files) removed, under
Tcl 6.7's own file names. The port shortened them for DOS (`RENAME.TXT`:
`tclBasic.c` became `BASIC.C`), but its sources still `#include` the
long names, which a case-sensitive host needs. It has all 27 C files
and headers of the release, and apart from whitespace 16 are identical
to the release's. The port:

- adds `msdos.c`, `msdos.h` and `tclerrno.h`;
- changes the Unix layer (`tclUnixAZ.c`, `tclUnixUtil.c`, `tclGlob.c`,
  `tclEnv.c`, `tclUnix.h`);
- elsewhere changes a few lines: `TCL_MSDOS_PORT` in `tcl.h`,
  `tclExpr.c` and `tclUnixStr.c`; `(int)` casts on pointer differences
  passed to `sprintf` in `tclCmdMZ.c`, `tclParse.c` and `tclUtil.c`; and
  the "end" index, `1<<14`, in `tclUtil.c`.

docs/M2-REPORT.md, section 3, has the details.

### Patches

`build/tcl67/src` is `build/tcl67/upstream` with `patches/tcl67/*.patch`
applied in order with `patch -p1 -F0` (Makefile). Patches 0002 to 0006
are M1's patches for Tiny Tcl carried over: the two candidates share
Tcl 6.7's interpreter core.

| Patch | Change |
|---|---|
| `0001-watcom-build.patch` | Builds with Open Watcom: Watcom headers for Borland's, `<errno.h>` for `tclerrno.h`, BIOS ticks for `biostime`, `readlink` and `geteuid` stand-ins, `mktemp`, `setenv`, a duplicate `errno` case |
| `0002-ansi-varargs.patch` | As M1's 0002: variadic functions with `<stdarg.h>` and prototypes |
| `0003-int16.patch` | As M1's 0003, and the port's own: its "no limit" of `1<<14` (`read` stopped at 16 KB), and `source` of 32 KB or more is an error |
| `0004-stack-guard.patch` | As M1's 0004 |
| `0005-sizes.patch` | As M1's 0005, unchanged |
| `0006-scan-long.patch` | As M1's 0006 |

## tcl16's own files

`src/main.c` (the `TCL.EXE` command line and REPL, M4),
`src/dostext.c` and `src/source.c` (DOS text and `source`, M4),
`src/file.c`, `src/glob.c`, `src/exec.c`, `src/scan.c`, `src/dosdir.c`
and `src/dosenv.c` (`file`, `glob`, `exec`, `scan`, directories and the
environment over libc16, M5; `glob.c` replaces Tiny Tcl's `tcldosgl.c`,
which is not built),
`src/tcl16.h`, `src/stack.c` (the stack guard), `src/probe.c` (the
`m1probe` measurement command), `src/tcl67main.c` (the M1/M2 test
driver, kept for the M2 reference build), `scripts/`, `tests/` and
`docs/` are written for tcl16. `tests/run-dos` is adapted from
gr-dos/perl16's `tests/run-dos`, and `tests/tools/err2out.c` is
perl16's; `tests/tools/child.c`, `cat.c`, `wc.c` and `pwd.c` are
tcl16's (M5). `scripts/fetch-libc16` is libc16's `consumers/fetch-libc16`
with the 0.15 pin, and `scripts/fetch-libkitchen16` libkitchen16's
`consumers/fetch-libkitchen16` with the 0.02 pin.

None of this work touches libc16. The libc16 experiment
(`scripts/try-libc16`) writes declaration-only shims inside
`build/libc16-try` to see past missing headers; they are not proposed for
libc16 (DESIGN.md, section 4).

## Toolchain and runtime

| Input | Pin |
|---|---|
| libc16 | release 0.15, `libc16-0.15.zip`, SHA-256 `042b6d2d18c381592cc929523835a695d7b2955a606bf44df6b2a43727f0d287` (`scripts/fetch-libc16`; M5; M4 pinned 0.07, M1 to M3 0.04) |
| libkitchen16 | release 0.02, `libkitchen16-0.02.zip`, SHA-256 `2ca356b7d2a23442b7851b10c09cfa577ee3888eaf5b4b899f10ee494162cb85` (`scripts/fetch-libkitchen16`; M5) |
| Open Watcom | open-watcom-v2 release `2026-10-01-Build`, `ow-snapshot.tar.xz`, SHA-256 `e6aa1b1e40ac8bbf97658d2c70fff8a4242d6ca4a1c60806f2baa5317083d4fe`, the snapshot libc16 names (its `tools/fetch-watcom`; unchanged from 0.04 to 0.15). Since M5 only its compiler and linker are used for `TCL.EXE`; the M2 reference build still links its runtime |
| DOSBox-X | the host's package, for the tests and measurements only |
| gcc | the host's, with `-m32`, for `make host` only |

M1 to M3 measured libc16 0.04 (M1-REPORT.md, section 6). M4 pins 0.07,
which has 0.05's `rename`, `unlink`, `remove`, `fseek` and `ftell` and
0.06's `strtol`, `strcspn`, `strstr`, `qsort`, `abort`, `clearerr`,
`isascii`, `fstat`, `getcwd` and `chdir`; DESIGN.md, section 4.2, has
what is still missing. M5 pins 0.15 and libkitchen16 0.02, and links
`TCL.EXE` with them (DESIGN.md, sections 4.2 and 5).
