# M1: Tiny Tcl 6.8.1 evaluation

Milestone M1 of [DESIGN.md](DESIGN.md), section 11: the Tiny Tcl 6.8
source audit, pinned source, build experiment and test report. It
follows the evaluation process of section 2.1 for the first candidate.
M2 does the same for the historical Tcl 6.7 DOS port, and M3 chooses
between them. This report does not choose.

> **Decided in M3.** Tcl16 is built on Tiny Tcl 6.8.1:
> [ADR-001-tcl-implementation.md](ADR-001-tcl-implementation.md) records
> the decision and its reasons. The rest of this report is unchanged,
> as the evidence for it.

> **Added in M2.** Evaluating the Tcl 6.7 DOS port, which shares this
> core, found more about Tiny Tcl ([M2-REPORT.md](M2-REPORT.md)):
>
> - **`scan`** stored integer fields in a 16-bit `int`, so
>   `scan 12345670 %o a` gave -13384. Tcl 6.7's own test suite found it.
>   `patches/0006-scan-long.patch` fixes it, and `tests/lang/ints` checks
>   it.
> - **DOS text scripts.** `source` reads scripts in binary mode. In a
>   script with DOS line ends (CRLF), a `\` continuation fails and a
>   string spanning lines keeps its CR, and a trailing ^Z runs as a
>   command. M1's tests are LF files, so they did not show this.
>   `tests/tinytcl/crlf` is a todo, and `tests/tinytcl/ctrlz` records
>   the ^Z behaviour.
> - **Candidate-specific tests** moved from `tests/lang` and `tests/dos`
>   to `tests/tinytcl` (`loop`, `readdir`, the 32 KB `source`, upper-case
>   `glob`, `pwd`'s `C:\`). The shared tests now pass on both
>   candidates. `make test` runs all three directories: 25 passed and 2
>   todo.
> - **Tcl 6.7's own suite** (`make test-suite`) passes 35 of 42 files.
>   The other 7 need `exec` for their fixtures.

## 1. Summary

- **Source.** Tiny Tcl 6.8.1 (September 2001), the last release, is
  vendored byte for byte and checked against the release archive's
  SHA-256 (PROVENANCE.md). It is Tcl 6.7 with TclX 6 pieces and a DOS
  I/O layer, about 24,500 lines of C. The license is the Tcl license,
  which is permissive.
- **Build.** With five patches (sections 4, 5 and 7), Open Watcom builds
  it for the large model with 8086 instructions. It runs the whole test
  suite on DOSBox-X's emulated 8086: 20 of 21 cases pass, and the 21st
  is a known gap in DOS path handling. The same sources built for Linux
  pass the same language tests, as a reference.
- **The upstream code was not 16-bit clean** with this compiler, despite
  its DOS heritage. Two classes of defect made it crash, and both are
  fixed:
  - Variadic functions had no prototypes, so under Open Watcom's
    register calling convention every error message corrupted memory
    (5.1).
  - Tcl 6 assumes a 32-bit `int` everywhere: in sentinels, `incr`,
    `format`, buffer doubling and size comparisons (5.2, 7.4).

  Stack exhaustion also corrupted memory, and allocation failure went
  unchecked. Deep recursion is now a Tcl error, and allocation failure
  stops the interpreter with a message (7.3, 7.4).
- **Limits that remain.** On DOS a single string or list holds at most
  32,767 bytes. A list element holds about 16 KB. The words of one
  command together hold 32 KB. With a 32 KB stack, recursion stops at
  about 50 levels of a simple proc, and at 15 when each level recurses
  inside `expr` (7.3). Integer arguments other than `expr`, `incr` and
  `format` are 16-bit. Past these limits the interpreter reports an
  error or stops with a message. It does not corrupt memory at any of
  the sites found, but the audit of `int`-sized lengths is not
  exhaustive.
- **libc16.** The interpreter core compiles unchanged with libc16 0.04's
  headers. Linking needs 20 functions and `environ` that libc16 lacks:
  - the `printf` family (`sprintf` alone has 16 call sites);
  - `strtod`, `strtol` and `sscanf`;
  - `qsort`, `opendir`/`readdir`/`closedir`, `getcwd`/`chdir`, `fstat`
    and `fseek`/`ftell`;
  - floating-point support, an 8087 emulator or FPU.

  The development build therefore uses Open Watcom's runtime, which is
  40% of its code (sections 6 and 7.1).
- **Footprint.** `TCL.EXE` is 139.5 KB, with a 168 KB load image. It
  takes 184 KB of conventional memory at start, and about 430 KB after
  the heaviest workload measured (7.2).

## 2. Source and provenance

PROVENANCE.md has the details. In short:

- `vendor/tinytcl-6.8.1/` is `tinytcl681.zip` (SHA-256 `6a4d24de…c0f4`)
  unmodified, with a per-file manifest. `make check-vendor-zip`
  re-downloads the archive and compares it.
- The release archives have no license file. The CVS repository's
  `LICENSE.txt`, the Tcl license, is kept as `vendor/LICENSE.tinytcl`.
  Each file carries the University of California, Lehenbauer/Diekhans or
  Henry Spencer notice. `dos.c`, `borland.c` and `regexp.h` have none.
- 6.8.0 differs from 6.8.1 only in files tcl16 does not build (plus a
  comment in `tcldos.h`). The CVS history ends in May 2001.

## 3. Source audit

### 3.1 Inventory

| Files | Lines | Role | In the M1 build |
|---|---|---|---|
| `tclBasic.c`, `tclParse.c`, `tclExpr.c`, `tclVar.c`, `tclProc.c`, `tclUtil.c`, `tclHash.c`, `tclGet.c`, `tclAssem.c`, `tclCmdAH.c`, `tclCmdIL.c`, `tclCmdMZ.c`, `panic.c`, `regexp.c` | 14,000 | The Tcl 6.7 core: parser, evaluator, `expr`, variables, procs, lists, strings, hash tables, regexps, and the built-in commands | yes |
| `tclEnv.c` | 506 | The `env` array, kept in step with `environ` | yes |
| `tcldosaz.c`, `tcldosut.c`, `tcldosst.c`, `tcldosgl.c`, `tcldos.h` | 3,400 | DOS versions of Tcl 6.7's Unix I/O commands: `open`, `puts`, `gets`, `file`, `glob`, `cd`, `pwd`, `source`, `exit` | yes |
| `tclXdbg.c`, `tclXgen.c`, `readdir.c`, `tclExtend.h`, `tclExtdInt.h` | 1,400 | TclX 6 pieces: `cmdtrace`, `loop`, `infox`, `readdir` | yes |
| `tclalloc.c` | 569 | `TCL_MEM_DEBUG` allocator (empty without it) | yes |
| `tinytcl.c` | 161 | Upstream's `main`: REPL, banner, sources `tcl_sys/autoinit.tcl` | no: `src/main.c` replaces it |
| `dos.c` | 1,072 | 26 Borland-specific DOS commands (`bios_memsize`, `com`, `kbhit`, `sound`, `gotoxy`, `video`, `execvp`…) using `<bios.h>`, `<conio.h>`, `<dir.h>` and `<alloc.h>` | no: left to M5 and M6, which design the DOS commands (DESIGN.md, section 8) |
| `borland.c` | 12 | `_stklen = 65400` for Borland | no: the stack is a link option |
| `tclunxaz.c`, `tcluxstr.c`, `tcluxutl.c`, `tclUnix.h`, `tclGlob.c`, `tclEmbed.c`, `tclTest.c` | 4,600 | Unix originals and test drivers that upstream's DOS build leaves out | no |

Upstream builds with Borland C 5.0 in the large model (`-ml -1-`), with
Borland's floating-point emulator and math library
(`tlink … mathl cl emu`).

### 3.2 Command set

The M1 build has 60 commands, and the development build adds
`m1probe`:

- **Tcl 6.7 core (42):** `append array break case catch concat continue
  error eval expr for foreach format global glob if incr info join
  lappend lindex linsert list llength lrange lreplace lsearch lsort proc
  regexp regsub rename return scan set split string trace unset uplevel
  upvar while`
- **I/O (14):** `cd close eof exit file flush gets open puts pwd read seek
  source tell`
- **TclX (4):** `cmdtrace infox loop readdir`
- **tcl16 measurement:** `m1probe`, in the development build only

Missing from what DESIGN.md, section 7, and section 8.1 expect:

- `exec` or any way to run a child program (only `dos.c`'s `execvp`,
  which is not built);
- `time`, `history`, `pid` and `switch` (Tcl 6 has `case`);
- `file delete`/`mkdir`/`rename`.

Every Required and High capability of section 7 is present: variables,
control flow, procs, `expr`, lists, strings, `source`, `catch`/`error`,
arrays, `eval`/`uplevel`/`upvar`, `format` and floating point.

### 3.3 Memory-model assumptions

- **Large model.** Pointers are far. Tcl stores them in `ClientData`
  (`void *`) and hashes one-word keys through a `long`
  (`RANDOM_INDEX`). Both are sound in the large model. No pointer is
  cast to `int` in the built files except `format`'s use of
  `oneWordValue`, which is fixed (5.2).
- **`int` is the size type.** String lengths, buffer sizes, list
  indices and counts are `int`, 16 bits here. This is the main source
  of defects (5.2, 7.4).
- **Allocation.** `ckalloc` is `malloc`, and nothing checks for NULL
  (7.4). Objects are separate small blocks from Open Watcom's far heap.
  No single object approaches 64 KB, because sizes stop at 32,767
  bytes.
- **Stack.** In the large model the stack shares DGROUP with the static
  data. Tiny Tcl's `MAX_NESTING_DEPTH` is 100 Tcl_Eval levels, but on an
  8086 the stack runs out first (7.3).
- **Unprototyped calls.** Tcl 6 is K&R C. With a register calling
  convention, every call that crosses an unprototyped boundary with
  different argument or return types is wrong. This happened in the
  variadic functions, `panic`, `getcwd` and `read` (5.1).

## 4. Build experiment with Open Watcom

`make` builds `build/dev/TCL.EXE` from `build/src`, which is the vendored
sources with LF line ends and `patches/` applied. It also compiles
`src/main.c`, `src/stack.c` and `src/probe.c`, and links with Open
Watcom's runtime:

    wcc -q -zq -ml -0 -zc -os -bt=dos ...
    wlink option stack=32k system dos ...

The flags are the large model, 8086 instructions, string literals in the
code segment (out of DGROUP) and size optimisation. Stack checking is on.

`patches/0001-watcom-build.patch` is the porting work proper:

- `#include "tclDos.h"` for a file named `tcldos.h`.
- Borland's `<values.h>`.
- `errno`, `sys_nerr` and `environ` redeclared differently from the
  library.
- `tclEnv.c`'s private `setenv`/`unsetenv` colliding with the
  library's.
- A `void` function returning a value.
- Open Watcom's `S_IFLNK`, on a system with no symbolic links.

This patch is not enough. The interpreter built with it alone crashes on
the first error message (5.1).

## 5. Defects found and fixed

### 5.1 Calling convention (patches/0002-ansi-varargs.patch)

`tcl.h` declares `Tcl_AppendResult`, `Tcl_SetErrorCode` and
`Tcl_VarEval` as `f()` when compiling C, and defines them with
`<varargs.h>`. Open Watcom passes the first arguments of an
unprototyped call in registers, but a `...` function reads them from the
stack. So every `Tcl_AppendResult(interp, "msg", …, NULL)`, which is
nearly every error message, read garbage pointers and wrote through
them.

Under DOSBox-X this appeared as writes into the ROM BIOS and "Corrupt
MCB chain". It hit `linsert` with a bad index, `incr` of a non-number,
and `glob` with no match. Borland's default stack convention had hidden
it.

The fix defines and declares the three functions with `<stdarg.h>` and
`(Tcl_Interp *interp, ...)`, as Tcl 7 does. `panic` gets the same fix:
it was a K&R function of nine fixed `char *` arguments, called with
fewer, and Open Watcom's callee-pops convention then corrupts the stack.

### 5.2 A 16-bit `int` (patches/0003-int16.patch)

Every item here was found by a test that passes on the 32-bit host
build and fails on DOS:

| Code | With a 16-bit `int` | Fix |
|---|---|---|
| `Tcl_GetListIndex` "end" = `1<<30`; `read` with no count `1<<30` | 0: `lreplace $l 1 end` replaced nothing, and `read $f` read nothing | `INT_MAX` |
| `lrange … end` = `1000000` | 16960 | `INT_MAX` |
| `Tcl_GetInt`: `int i = strtol(…)` | silent wrap: `lindex $l 65536` was `lindex $l 0` | range check: "integer value too large to represent" |
| `incr` in `int` | 32767 + 1 = -32768; `incr x 70000` wrapped | `long`, through the new `TclGetLong`, as `expr` does |
| `format` stores `%d`'s int in the low half of a far `char *`, and `%D` passes the pointer as a `long` | `format "%s %D" abc 5` printed the stale segment; `format %d 40000` was impossible | every integer field read with `TclGetLong` and formatted as `%l…` |
| `source`: `malloc((unsigned) st_size + 1)`, `read(…, (int) st_size)` | a 65,535-byte script allocated 0 bytes and overran the heap; a 70,000-byte one was cut to 4,464 bytes silently | scripts of 32,767 bytes or more go to upstream's line-by-line reader (`Tcl_SmallFootprintEvalFile`) |

So `expr`, `incr` and `format` work in 32 bits on DOS, as on a 32-bit
host. Other integer arguments (list and string indices, `format` widths)
are 16-bit and give an error when out of range (`tests/lang/int16.tcl`).

## 6. libc16 build experiment

`make libc16-try` (`scripts/try-libc16`) compiles the same sources with
libc16 0.04's headers only, in the large model, as libc16's consumers do
(`-zl -zld -x -ml -0 -s`). It then links them with `LIBC16L.LIB` alone.
`build/libc16-try/report.txt` has the full output.

1. **Compiling.** The 15 core modules, `src/main.c` and `src/stack.c`
   compile unchanged. The 8 I/O and TclX modules fail only for want of
   `<dirent.h>` and `<signal.h>` (and `environ` in `tclEnv.c`). With
   declaration-only shims for those, everything compiles.
2. **Linking** leaves these undefined. None is in libc16 0.04:

   | Need | Functions | Used for |
   |---|---|---|
   | Formatted output | `sprintf` (16 call sites), `fprintf`, `printf`, `vfprintf` | Every number-to-string conversion, `format`, error messages, `panic` |
   | Formatted input and conversion | `strtod`, `strtol`, `sscanf` | `expr`, `Tcl_GetInt`, `Tcl_GetDouble`, `scan` |
   | Floating point | 8087 fixups `FIxRQQ`/`FJxRQQ` (6), `__CHP` | `expr` doubles, `scan %f`, `format %f`, hash statistics |
   | Directories | `opendir`, `readdir`, `closedir` | `glob`, `readdir` |
   | Files and directories | `fstat`, `fseek`, `ftell`, `getcwd`, `chdir`, `clearerr` | `source`, `seek`, `tell`, `pwd`, `cd`, the REPL |
   | Other | `qsort`, `strcspn`, `isascii`, `abort`, `environ` | `lsort`, `regexp`, `panic`, `env` |

   In the 0.05 checkout of the libc16 repository, only `fseek`/`ftell`
   exist, and `strtol` is mentioned in `docs/spec/format.md`. 0.05 was not
   released at the time of M1.

3. **Convention trap.** libc16's functions are `__cdecl`. Two kinds of
   call compile to Open Watcom's register convention instead (`name_`
   in the table above):
   - calls to functions libc16's headers do not declare (`fstat`,
     `getcwd`, `fseek`, `ftell` and the `printf` family);
   - calls to functions that only upstream's `tcldos.h` declares
     (`chdir`, `mktemp`, `getwd`).

   Where a libc16 header declares a function first (`access`, `open`,
   `close`, `unlink`), the call links as libc16's `_name`. A libc16
   build must use only libc16's declarations for anything libc16
   provides, or it repeats defect 5.1.

Options for M3 and M4 (DESIGN.md, sections 4 and 5). This report does
not decide among them:

- **`opendir`/`readdir`.** A small Tcl16-local adapter over libc16's
  `_dos_findfirst`/`_dos_findnext`.
- **`sprintf`.** Tcl's own uses are a short list of fixed formats
  (`%d`, `%ld`, `%u`, `%s`, `%g`, `%.150s`), so a small Tcl16-local
  formatter covers them. The `format` command needs the general case,
  including `%e`/`%f`/`%g`. That is the open question of section 5,
  which section 7 sizes.
- **Floating point** is the largest item. It needs either an 8087
  emulator linked in (Open Watcom's `emu87` is not libc16 material),
  integer-only `expr`, or a soft-float library. DESIGN.md, section 7,
  already treats it as "evaluate separately".
- **Anything added to libc16** must be clean-room (DESIGN.md, section 4).
  Nothing here was added to libc16.

## 7. Measurements

`make measure` builds five variants (`scripts/build-variant`) and runs
the probe scripts in `tests/probe` under DOSBox-X with each
(`scripts/measure`). It writes `build/measure/report.txt`. The variants
are:

| Variant | Build |
|---|---|
| `os` | `build/dev/TCL.EXE`: `-os` (size), 32 KB stack |
| `ox` | `-ox` (speed), 32 KB stack |
| `od` | `-od` (no optimisation), 32 KB stack |
| `stk16`, `stk48` | `-os`, 16 KB or 48 KB stack |
| `os+nofpu` | `os`, run with DOSBox-X's 8087 turned off |

The emulator runs an 8086 core at a fixed 315 cycles per millisecond,
about a 4.77 MHz IBM PC, as gr-dos/perl16 does. The memory, size and
stack figures are exact for this build under DOSBox-X's DOS. The
timings are emulator estimates, good for comparing variants and
candidates, not real hardware.

### 7.1 Sizes

| KB | `os` | `ox` | `od` | `stk16` | `stk48` |
|---|---|---|---|---|---|
| `TCL.EXE` | 139.5 | 139.2 | 187.6 | 139.5 | 139.5 |
| Load image | 168.1 | 169.3 | 215.2 | 152.1 | 184.1 |
| Code: Tiny Tcl and tcl16 | 78.7 | 80.0 | 125.8 | 78.7 | 78.7 |
| Code: Open Watcom runtime | 51.6 | 51.5 | 51.6 | 51.6 | 51.6 |
| DGROUP data (without the stack) | 5.9 | 5.9 | 5.9 | 5.9 | 5.9 |
| Stack | 32 | 32 | 32 | 16 | 48 |

- **Code.** The runtime is 40% of the code. Its largest piece is the
  8087 emulator, `emu87.lib`, at about 10.6 KB. Next come the
  floating-point conversions (`math87l.lib`), `printf`/`scanf`, stdio,
  the heap and `qsort`. Open Watcom's `strtod`/`stat`/`tzset` also pull
  in time-zone and multibyte code that Tcl does not need. A libc16
  build replaces all of this, so its size depends on the answers to
  section 6.
- **Tiny Tcl's own code** is 78.7 KB at `-os`. The largest modules are
  `tclVar` (9.2 KB), `tclCmdMZ` (8.7), `tcldosaz` (7.9), `tclCmdIL`
  (7.1) and `tclExpr` (5.5).
- **DGROUP.** Static data is small (5.9 KB), because `-zc` puts string
  literals in the code segments. The stack is therefore free to grow
  toward 58 KB if M3/M4 want deeper recursion (7.3).
- **`-ox`** is 1.2 KB larger in memory and about 15% faster (7.5).
  **`-od`** is 47 KB larger and slower, so it is only for debugging.

### 7.2 Conventional memory

DOSBox-X's DOS leaves 588.6 KB free to a program. Each figure is what
`TCL.EXE` has taken from DOS at that point (`os`; `ox` is 1.2 KB more).
The workloads in `tests/probe/work.tcl` accumulate:

| After | KB taken |
|---|---|
| Start (load image 168 KB, plus the initial heap) | 183.5 |
| Procedure definitions and `fib 10` | 183.5 |
| A 3,000-element list built with `lappend` | 271.6 |
| A 1,000-element array | 279.6 |
| A 30,000-byte string built by doubling | 344.2 |
| A copy of it (`set t $s`) | 428.9 |
| `unset` of all of them | 428.9 |

- **Small machines.** 184 KB at start means a 256 KB machine is marginal.
  The workloads above need about 430 KB free, a 512 KB machine.
- **The heap never shrinks.** Open Watcom's heap does not give memory
  back to DOS, so `unset` frees nothing as DOS sees it. It is reused
  inside Tcl, but DESIGN.md's child-process plans (`exec`, M5) will find
  less memory than at start.
- **Growth by doubling** is costly. The 3,000-element list (about 27 KB
  of text) took 88 KB, and the 30,000-byte string took 65 KB: the old
  and new buffers coexist during each copy, and the freed blocks are
  holes that larger objects cannot reuse. Strings and lists near 32 KB therefore need 2 to 3
  times their size.

### 7.3 The stack (patches/0004-stack-guard.patch)

The upstream interpreter has no stack limit except `MAX_NESTING_DEPTH`
(100 nested `Tcl_Eval` calls). Without the patch, `proc count {n}
{return [expr {1 + [count [expr {$n + 1}]]}]}` overran the 32 KB stack
into the static data and heap after about 15 levels. The build has Open
Watcom's stack checks, but they did not catch it. The run wrote to the
ROM BIOS and never finished, and the `catch` around it never returned.

`TclStackExhausted` (`src/stack.c`) compares `stackavail()` against a
3,072-byte margin. `Tcl_Eval` calls it on entry and fails with "too many
nested calls to Tcl_Eval (out of stack)", which `catch` can trap. The
margin covers the deepest C path between two `Tcl_Eval` calls that the
tests reach (`expr` parsing and `format`). It was not derived from a
whole-program analysis.

| | `stk16` | `os` (32 KB) | `stk48` |
|---|---|---|---|
| Stack free when the script starts (bytes) | 15,189 | 31,573 | 47,957 |
| Deepest simple recursion (`proc down {n} {…; down [expr …]}`) | 23 | 53 | 84 |
| Deepest recursion inside `expr` (`expr {1 + [count …]}`) | 6 | 15 | 23 |
| Stack per level of `return [stk [expr …]]` (bytes) | 954 | 954 | 954 |

Each level of Tcl recursion costs several nested `Tcl_Eval` calls in C
(the body, the bracketed command, `expr`'s own evaluation), at roughly
500 to 700 bytes each.

The practical limit is the second row. `fib 15` needs 15 levels and
fails with a 16 KB stack (and in the `od` build at 32 KB). Programs
written for desktop Tcl, which recurse hundreds of levels, will not run.
Raising the stack to the DGROUP limit (about 58 KB) would allow roughly
30 levels inside `expr`. Moving Tcl's large local buffers (several
functions have 200 to 500 byte `char` arrays on the stack) to the heap
is the other lever, for M3/M4 to weigh.

### 7.4 Object sizes and allocation failure (patches/0005-sizes.patch)

Tcl 6 grows its buffers by doubling an `int` size and compares sizes as
`int`. With 16 bits, a string passing 16 KB asks to double to a
negative size or to a size smaller than needed. `malloc` then returns
too little (or NULL, which nothing checked), and the copy runs off the
end of the block. `append` past 16 KB, `lappend` and `concat` near
32 KB, and `format` with a large width all corrupted the heap this way.

The patch adds two functions in `panic.c`:

- `TclGrowSize(need, want)` computes the new size in `long`. If the
  need passes `INT_MAX` it stops the interpreter with "tcl16: string or
  list too long".
- `TclCkalloc`, now `ckalloc`, stops with "tcl16: out of memory" when
  `malloc` returns NULL.

These are used in variables, the result, `Tcl_Merge`/`Tcl_Concat`,
list element quoting, `format` and the parser. Both stops call `abort`
after flushing standard output (exit status 255; `tests/dos/toolong`).
Stopping rather than returning a Tcl error is deliberate: the sites are
deep in code that has no error path, and a clean stop is what DESIGN.md,
section 5, asks for when the choice is a stop or corruption.

The resulting limits on DOS:

| Object | Limit |
|---|---|
| A string value, a list's text, the interpreter result | 32,767 bytes |
| One list element, quoted with braces or backslashes | about 16 KB (quoting may double it) |
| All the words of one command, after substitution (they share one parse buffer) | 32,767 bytes, so `string compare $a $b` of two 20 KB strings stops |
| A script given to `source` | any size: files of 32,767 bytes or more are read command by command (0003) |
| One command in a script, including a `proc` body | 32,767 bytes |

An allocation failure in a well-formed script is untested. The out of
memory stop exists, but no test drives the heap to exhaustion.

### 7.5 Speed

BIOS ticks (55 ms each) for each benchmark in `tests/probe/bench.tcl`,
at about 4.77 MHz-equivalent speed. "error" means the benchmark ran out
of stack (7.3). The `stk16` and `stk48` timings match `os` and are left
out of the table.

| Benchmark | `os` | `ox` | `od` | `os+nofpu` |
|---|---|---|---|---|
| `loop1000`: 1,000 empty `for` iterations | 278 | 238 | 324 | 278 |
| `expr1000`: … with `set x [expr {$i * 3 + 7}]` | 606 | 518 | 703 | 606 |
| `fexpr1000`: … with `expr {$i * 1.5 + 0.25}` | 753 | 665 | 854 | 1,239 |
| `call500`: 500 calls of an empty proc | 173 | 147 | 210 | 172 |
| `fib15`: recursive Fibonacci, 1,973 calls | 1,384 | 1,173 | error | 1,385 |
| `lappend2000` | 815 | 695 | 985 | 814 |
| `lsort500` (building the list included) | 255 | 213 | 306 | 256 |
| `string500`: `append` of `string toupper` | 228 | 198 | 277 | 228 |
| `regexp200` with two subexpressions | 118 | 98 | 147 | 118 |
| `format500` of `"%5d %s %x"` | 315 | 284 | 363 | 314 |

- **Absolute speed is low**, as expected of a string interpreter on an
  8088:
  - one empty `for` iteration costs about 15 ms;
  - an integer `expr` with a substitution about 18 ms more;
  - a proc call about 4 ms;
  - `fib 15` takes 76 seconds.

  These are interactive and scripting speeds, not compute speeds.
- **`-ox` is 10 to 17% faster than `-os`**, for 1.2 KB. `-od` is 13 to
  25% slower than `-os`.
- **Floating point.** With DOSBox-X's 8087, a float `expr` costs 8 ms
  more than an integer one. Without the 8087, Open Watcom's emulator
  makes it about 35 ms more, nearly three times as slow overall
  (`fexpr1000` is 1,239 ticks against 606 for integers). Only floating
  point is affected. Integer `expr` keeps to integers in Tcl 6, so
  `os+nofpu` is otherwise the same as `os`.

## 8. Tests

`make test` runs every case in `tests/lang` and `tests/dos` under
DOSBox-X, one session per case, with an 8086 core and CGA machine
(`tests/run-dos`). A case is a script and its expected standard output.
Optional headers give the exit status (`# status:`), the command line
(`# command:`) or a known failure (`# todo:`). The runner also fails a
case if DOSBox-X's log shows an exit through an exception or a write to
ROM, which is how the memory corruption in section 5 first showed up.

`make test-host` runs `tests/lang` with `build/host/tclhost`, the same
patched sources built with gcc `-m32` (`scripts/build-host`). Its
32-bit `int` makes it the reference. Most expected outputs were
generated with it and then checked by hand against Tcl 6.7's manual
pages.

| Suite | Cases | Covers |
|---|---|---|
| `tests/lang` | 12 | quoting and substitution, `expr` (integer, float, string compare), control flow, procs and `upvar`/`uplevel`, lists, strings, arrays, errors and `errorInfo`, `eval`/`source`/`info`/`trace`, 32-bit `incr`/`format`, the 16-bit argument limits (DOS only), 30 KB strings and lists |
| `tests/dos` | 9 | files (`open`/`gets`/`read`/`seek`/`eof`/`file`/`glob`), `env`, `argv`, `exit` status, a script error's status, `cd`/`pwd`, deep recursion (7.3), the 32 KB stop (7.4), DOS paths |

Results with the M1 sources:

- DOS (`build/dev/TCL.EXE`): 20 passed, 0 failed, 1 todo (`dospath`).
- Host: 11 passed, 1 skipped (`int16`, which is DOS only).

The todo is `tests/dos/dospath`. `file tail` and `file dirname` split
only at `/`, so `file dirname {C:\DIR\NAME.EXT}` is `.` instead of
`C:\DIR`, and `file rootname {C:\DIR.X\NAME}` is wrong. `pwd` returns
backslashes (`C:\`), so scripts that take a path from `pwd` and split it
are affected. This is upstream behaviour left for M3/M4. Fixing it means
making Tcl's path commands recognise `\` and drive letters. It is the
kind of DOS-specific deviation DESIGN.md, section 6, asks to document.

The tests do not cover:

- the interactive prompt;
- `glob` patterns across directories;
- `regsub` beyond the basics;
- `cmdtrace`;
- floating-point formatting edge cases;
- behaviour on real hardware;
- other DOS kernels: tests use DOSBox-X's DOS, not GR-DOS.

## 9. Compatibility notes

Tiny Tcl is the Tcl 6.7 language. Scripts written for Tcl 7 or 8 need
these changes, which the tests document:

- `expr` takes exactly one argument. Brace it, as in `expr {$a + 1}`.
- There are no math functions (`int()`, `sqrt()`…). Integer division
  truncates toward zero (`-7/2` is -3), as C does.
- There are no `\x` or `\u` escapes, and no `switch`, `time`, `exec`,
  `pid`, `history`, `clock`, `file delete`, `file mkdir`, `namespace`,
  `dict` or `lassign`. `case` takes the place of `switch`.
- `string range` and `lrange` report errors in their own words.
- Integers are 32-bit in `expr`, `incr` and `format`, and 16-bit
  elsewhere (5.2). `format %u` of a value over 2³¹ is an error.
- Output to a file or the console is in DOS text mode: `\n` is written
  as CR LF. `read` and `gets` return LF.
- Error messages from the C library, such as "No such file or
  directory", are Open Watcom's text and may differ with libc16.
- Tiny Tcl's `putenv` replaces the C library's, so `env` changes reach
  child programs once there are any.
- `exit` in a script sets the DOS exit status. An uncaught script error
  prints `Error:` and `errorInfo` to standard error and exits with 1.

`src/main.c` is a test driver. It does not source upstream's
`tcl_sys/autoinit.tcl`, sets no `tcl_library` and prints no banner. The
startup and command line of DESIGN.md, sections 6 and 8, are M4's job.

## 10. Assessment

This is evidence for M3, not a choice.

**For Tiny Tcl.**

- A permissive license and a small, readable code base (14,000 lines of
  core).
- Every Required and High capability of DESIGN.md, section 7, exists
  and passes on an emulated 8086.
- The large-model memory use (184 KB at start) leaves room on a 512 KB
  or 640 KB machine.
- The source is pinned, unmodified, with checked provenance.

**Against, or work still owed.**

- **It was not 16-bit clean.** Five patches (about 440 changed lines)
  were needed before it stopped corrupting memory. The `int`-as-size
  audit was driven by tests and is not exhaustive. Other sites may
  remain where a computed size passes 32 KB.
- **The stack limits recursion.** Fifteen levels inside `expr` is low
  for real scripts (7.3).
- **Object sizes cap at 32 KB.** Strings and lists stop there, and past
  the limit the interpreter stops rather than raising a Tcl error
  (7.4).
- **libc16 is far from enough.** The `printf` family, `strtod`/`strtol`,
  floating point and directory reading are the gap (6). Floating point
  in particular means carrying an 8087 emulator in some form, or
  dropping floats from `expr`.
- **DOS integration is thin.** There is no `exec`, and DOS paths are not
  handled (8). `dos.c`'s commands are Borland-specific and would be
  rewritten, not ported.

M2 should measure the historical Tcl 6.7 DOS port with the same tests,
`scripts/measure` and libc16 experiment. Both candidates share the Tcl
6.7 core, so most of patches 0002 to 0005 should carry over. The
comparison is mainly the DOS layer, provenance and licensing.

## Reproducing

    make check-vendor-zip   # the vendored source is the release
    make test test-host     # section 8
    make libc16-try         # section 6: build/libc16-try/report.txt
    make measure            # section 7: build/measure/report.txt
