# M2: Tcl 6.7 DOS port evaluation

Milestone M2 of [DESIGN.md](DESIGN.md), section 11: the Tcl 6.7 DOS port
source audit, pinned source, build experiment and test report. It
follows the evaluation process of section 2.1 for the second candidate,
with the same tests, measurements and libc16 experiment as
[M1-REPORT.md](M1-REPORT.md) for Tiny Tcl 6.8.1, so that M3 can compare
them. This report does not choose.

## 1. Summary

- **Source.** The candidate is Peter Sprenger's April 1993 MS-DOS port
  of Tcl 6.7. Its archive is still published in the Tcl project's
  SourceForge area. It is stock Tcl 6.7 with a DOS layer: `msdos.c`, a
  reworked Unix layer and a few casts.
  - It is **fetched and checked, not vendored**. The port's own files
    carry no license, and the archive includes a Borland copyrighted
    header and third-party files of unknown terms (section 2).
  - The rest is Tcl 6.7 under the University of California's notice.
- **Same core as Tiny Tcl.** Tiny Tcl's interpreter core is Tcl 6.7, and
  most of its files differ from the release only in RCS headers. So
  every core defect M1 found is here too.
  - M1's patches 0002 to 0005 carry over almost unchanged.
  - Running Tcl 6.7's own test suite found one more defect shared by
    both, `scan` into a 16-bit `int`. It is fixed in both (patch 0006).
- **Build.** With six patches, Open Watcom builds the port for the
  large model and 8086 instructions. Sprenger's README asks for
  Borland's huge model, but the large model works because no object
  reaches 64 KB.
- **Tests.** The port passes the shared portable tests and its own
  cases: 27 passed and 1 todo, the same DOS-path gap as Tiny Tcl.
  On Tcl 6.7's own suite, both candidates give identical results once
  DOS line ends are normalised: 35 of 42 files pass. The other 7 cannot
  run their `exec`-made fixtures under DOSBox-X (section 8).
- **Where the candidates differ** is the DOS layer (section 6):

  | | Tcl 6.7 DOS port | Tiny Tcl 6.8.1 |
  |---|---|---|
  | Child programs | `exec` through COMMAND.COM, with redirection | none |
  | Other commands | `time`, `history` | TclX `loop`, `cmdtrace`, `readdir`, `infox` |
  | DOS text scripts (CRLF, ^Z) | read correctly | CRLF breaks continuations; ^Z is read as a command |
  | Path separator | `pwd` gives `C:/` | `pwd` gives `C:\` |
  | Glob case | lower case | upper case |
  | Scripts of 32 KB or more | an error | read command by command |
- **The port's DOS layer has defects of its own**, which these patches
  leave as found:
  - `exec` builds its command line in fixed buffers without bounds
    checks; a 300-character argument hangs the interpreter.
  - `exec` of a missing program succeeds, with the shell's message as
    its result.
  - `exec` depends on the shell's exit status.
  - `to_bslash`/`to_slash` copy paths into 129-byte static buffers.
- **libc16.** The core compiles unchanged with libc16 0.04's headers,
  as for Tiny Tcl. Linking needs the same 20 functions, `environ` and
  floating point, plus four more for the port's extra commands:
  `system`, `_mktemp` and `unlink` for `exec`, and `strstr` for
  `history` (section 9).
- **Footprint.** The two run at the same speed and recurse to the same
  depth, as they share the core. The port's `TCL.EXE` is 12 KB larger
  (152 KB against 140 KB at `-os`) and it takes 9 KB more conventional
  memory at start. Most of the difference is `exec`'s runtime, the
  spawn and `system` code (section 7).

## 2. Source and provenance

PROVENANCE.md, "Tcl 6.7 DOS port", has the details: URLs, the three
SHA-256 values (outer archive, inner `tcl67dos.zip`, and the Tcl 6.7
release used for comparison), and what the archive holds.

What matters for M3:

- **No license for the port itself.** `MSDOS.C` and `MSDOS.H` say "MSDOS
  TCL port by Peter Sprenger" and nothing else. The README gives the
  author's 1993 e-mail addresses. Choosing this candidate means either
  getting permission or replacing those two files. They are 720 and
  150 lines, and `exec` is most of them.
- **A Borland header.** `TCLERRNO.H` is Borland's `errno.h`, all rights
  reserved. The build does not use it.
- **Third-party files of unknown terms.** `UTIL/` and `DOC/` come from
  John S. Martin's Tcl 6.2 port, and the README says "There was no
  copyright message, so I think it is okay". tcl16 uses only the
  fetched copy, to run Tcl 6.7's test suite.
- **Everything else** is Tcl 6.7 under the University of California's
  notice, which permits use, modification and distribution with the
  notice kept.

So `make` fetches the archive (`scripts/fetch-tcl67`), checks it and
builds it under `build/`, and the repository holds only tcl16's patches
(the user's decision for M2).

## 3. Source audit

### 3.1 Inventory

| Files | Lines | Role | In the build |
|---|---|---|---|
| `tclBasic.c`, `tclParse.c`, `tclExpr.c`, `tclVar.c`, `tclProc.c`, `tclUtil.c`, `tclHash.c`, `tclGet.c`, `tclAssem.c`, `tclCmdAH.c`, `tclCmdIL.c`, `tclCmdMZ.c`, `tclHistory.c`, `tclCkalloc.c`, `panic.c`, `regexp.c` | 15,700 | The Tcl 6.7 core, the same code as Tiny Tcl's plus `history` | yes |
| `msdos.c`, `msdos.h` | 870 | Sprenger's DOS layer: `exec`, `open`, `time`, `~` expansion, path conversion | yes |
| `tclUnixAZ.c`, `tclUnixUtil.c`, `tclUnixStr.c`, `tclGlob.c`, `tclEnv.c`, `tclUnix.h` | 3,600 | Tcl 6.7's Unix I/O commands, changed for DOS: `file`, `glob`, `cd`, `pwd`, `source`, `env` | yes |
| `tclTest.c` | 155 | Upstream's test driver | no: `src/main.c` replaces it |
| `tclerrno.h` | 85 | Borland's `errno.h` | no |

The port's Borland project, `TCL.PRJ`, lists the same 22 modules.

### 3.2 What the port changed in Tcl 6.7

Apart from whitespace, 16 of Tcl 6.7's 27 C files and headers are
unchanged. The changes:

- **`tcl.h`, `tclExpr.c`, `tclUnixStr.c`**: `TCL_MSDOS_PORT`, defined in
  `tcl.h`, selects `msdos.h` in place of `tclUnix.h`.
- **`tclCmdMZ.c`, `tclParse.c`, `tclUtil.c`**: `(int)` casts on pointer
  differences passed to `sprintf`, because Borland's huge pointers
  subtract to a `long`. The README calls this out. With Open Watcom's
  large model they are `int` anyway.
- **`tclUtil.c`**: "end" as `1<<14` in place of `1<<30`. The same
  sentinel in `tclUnixAZ.c` limited `read` with no count to 16 KB
  (section 5).
- **`tclUnixUtil.c`**: `Tcl_EvalFile` no longer insists that `read()`
  return the file's size, because text mode shrinks CRLF. It puts the
  terminating zero at the count read. The Unix process functions are
  removed.
- **`tclUnixAZ.c`, `tclGlob.c`**: paths are converted with
  `to_bslash`/`to_slash` (`_BACKSLASH_PORT_`). `readdir` results are
  made lower case. `~user` is not supported.
- **`tclEnv.c`**: drops Tcl's own `putenv`, using the library's.
- **`msdos.c`** carries the rewritten functions:
  - `Tcl_OpenCmd`: no pipelines;
  - `Tcl_TimeCmd`: BIOS ticks;
  - `Tcl_ExecCmd`: `system()`;
  - `Tcl_TildeSubst`: `HOME` only;
  - stubs for the process functions.

### 3.3 Memory-model assumptions

- **Huge model, per the README.** "Remember to use huge pointers,
  because the Tcl-library was not developed to behave correctly under
  segment pointers." With the 16-bit size fixes of patches 0003 and
  0005, no Tcl object can reach 64 KB. The large model's far pointers,
  normalised by `malloc`, are therefore enough. The full suite passes
  in the large model, and huge-model arithmetic would only cost speed.
- **Stack.** The README asks for 12 KB and Borland's stack check. The
  M2 build uses 32 KB and tcl16's guard (patch 0004). The figures are
  in section 7.
- **Optimisation.** The README reports hangs with Borland's optimiser.
  Open Watcom's `-os` and `-ox` builds both pass.
- **Unprototyped calls and 16-bit `int`**: the same as Tiny Tcl's (M1
  sections 3.3, 5.1 and 5.2), in the same code.

### 3.4 Command set

The M2 build has 59 commands, plus `m1probe` in the development build:

- **Tcl 6.7 core (42):** `append array break case catch concat continue
  error eval expr for foreach format global history if incr info join
  lappend lindex linsert list llength lrange lreplace lsearch lsort proc
  regexp regsub rename return scan set split string trace unset uplevel
  upvar while`
- **I/O and system (17):** `cd close eof exec exit file flush gets glob
  open puts pwd read seek source tell time`

Compared with Tiny Tcl's 60:

- the port adds `exec`, `time` and `history`;
- it lacks TclX's `loop`, `cmdtrace`, `infox` and `readdir`.

Neither has `switch`, `pid`, `clock`, `file delete`, `file mkdir` or
`namespace`.

## 4. Build experiment with Open Watcom

`make tcl67` builds `build/tcl67/dev/TCL.EXE`. It uses the same flags,
runtime and stack as M1's `build/dev/TCL.EXE`, with
`patches/tcl67/*.patch` and `-DTCL16_TCL67` for `src/main.c`.

The port built from Sprenger's sources as he left them fails in three
ways:

- `msdos.h` includes Borland's `<dir.h>`, `<alloc.h>`, `<bios.h>` and
  `<sys\stat.h>`, and the Borland `tclerrno.h`.
- Six files end with a DOS end-of-file byte (^Z), which Open Watcom
  reads as source. `scripts/fetch-tcl67` removes it with the CRs.
- `msdos.c` sets Borland's `_fmode` and `_stklen` and calls
  `biostime`.

`patches/tcl67/0001-watcom-build.patch` fixes these, and also:

- `tclEnv.c`'s `setenv`, the same collision as Tiny Tcl's;
- prototypes for the Unix stand-ins, plus a `readlink` that fails with
  `EINVAL`;
- Open Watcom's `EDEADLOCK`, which equals `EDEADLK`.

With 0001 alone, the port runs simple scripts but crashes on the first
error message, exactly as Tiny Tcl did (section 5).

## 5. Defects found and fixed

| Patch | Defect | Relation to M1 |
|---|---|---|
| 0002 | Variadic functions without prototypes: `catch {incr x y}` wrote into the ROM BIOS and lost the script's output | M1's 0002, in the same code |
| 0003 | 16-bit `int` throughout | M1's 0003, plus the port's own below |
| 0004 | No stack limit but `MAX_NESTING_DEPTH` (100) | M1's 0004, unchanged |
| 0005 | Unchecked `malloc`, buffer growth past 16 KB | M1's 0005, applied unchanged |
| 0006 | `scan` stores `%d`/`%o`/`%x` in an `int`: `scan 12345670 %o a` gave -13384 | New in M2, in both candidates (M1 gets patches/0006 too) |

The port's own 16-bit issues, in 0003:

- **`read $f` with no count read at most 16,384 bytes**, silently. The
  port lowered Tcl's "no limit" from `1<<30` to `1<<14`, so it is a
  16-bit sentinel, but a small one. Now `INT_MAX`.
- **`source`** allocated `(unsigned) st_size + 1` bytes and wrote the
  terminating zero at whatever `read()` returned. A 65,535-byte script
  allocated nothing, and a read error (-1) wrote before the block.
  Tcl 6.7 has no line-by-line reader like Tiny Tcl's, so a script of
  32,767 bytes or more is now an error: "file too large".

Sprenger's README mentions the `scan` problem: "some tests with the Tcl
scan function failed out of this reason". Tcl 6.7's suite still fails
`scan-1.21` to `1.24` in both candidates without 0006.

Not fixed, because they are in the DOS layer that M5 would redesign
(section 6):

- `exec`'s fixed buffers;
- `to_bslash`'s 129-byte buffer;
- the `(int)` casts on far pointer differences.

## 6. The DOS layer compared

Each item has a test in `tests/tcl67` (and `tests/tinytcl`):

- **`exec`** (`exec.tcl`) joins its words into a COMMAND.COM command
  line, with `<`, `<<`, `>` and `>>`. `<<` goes through a temporary
  file. Output goes to a temporary file, and the result is read back
  from it. `|` is passed to the shell. The design has these limits:
  - **The status is the shell's.** `exec` sees only COMMAND.COM's
    exit status, and the shell's own messages are captured like any
    output. Under DOSBox-X, `exec nosuchpr` succeeds, with the result
    `Bad command or filename - "nosuchpr"`.
  - Under DOSBox-X's own shell, **every external program reports
    failure**. `COMMAND /C FREEMEM` exits nonzero even though FREEMEM
    exits 0, and a plain Open Watcom `system("FREEMEM")` gets 1. Only
    shell built-ins (`echo`, `ver`, `mkdir`) work in the tests. Real
    DOS and GR-DOS have to be checked in M7; GR-DOS's COMMAND.COM does
    not run on DOSBox-X's kernel.
  - **Buffers.** `command[200]`, `input[40]`, `output[40]`, with
    `strcat`/`sprintf` and no checks. `exec echo` of a 300-character
    word hangs the interpreter. DOS limits a command tail to 126
    characters anyway.
  - **Memory.** A child gets what is left of conventional memory, less
    COMMAND.COM. The interpreter's heap never shrinks (M1 section 7.2),
    and nothing frees memory before the call.
  - **No pipes from `open`**: `open "|cmd"` is an error.
- **Paths** (`pwdsep.tcl`). With `_BACKSLASH_PORT_`, `pwd` returns
  `C:/` and names given to Tcl may use either separator. `file dirname`
  and `file tail` still split only at `/`, so `C:\DIR\NAME.EXT` is a
  single component, and the common `dospath` todo fails here too, in a
  different way from Tiny Tcl.
- **Glob case** (`glob.tcl`). `readdir` names are made lower case, and
  patterns match case-sensitively, so `glob *.TXT` finds nothing; the
  README says "you HAVE to use lowercase". Tiny Tcl is the opposite: it
  returns upper case and needs upper-case patterns. Neither matches
  case-insensitively, as DOS does.
- **Text scripts** (`crlf.tcl`, `ctrlz.tcl`). The port reads scripts in
  text mode, so CRLF becomes LF and a trailing ^Z ends the file. Tiny
  Tcl reads in binary mode: a `\` continuation in a CRLF script fails,
  a string spanning lines keeps its CR, and a trailing ^Z runs as a
  command. M1 did not see this because its tests are LF files. It
  matters for scripts written with DOS editors.
- **Big scripts** (`bigsrc.tcl`). A script of 32 KB or more is an error
  in the port. Tiny Tcl reads it command by command.
- **`time`** (`timehist.tcl`) counts BIOS ticks, 55 ms each, and
  `history` is Tcl 6.7's.
- **Not in the port:** TclX (`loop`, `cmdtrace`, `infox`, `readdir`) and
  Tiny Tcl's `dos.c` commands (which M1 did not build either).

## 7. Measurements

`make measure-tcl67` measures the same five variants with the same
probes as M1 (M1-REPORT.md, section 7), under DOSBox-X at a fixed 315
cycles per millisecond. The Tiny Tcl figures here are from `make
measure` with its current patches (0006 included), run at the same time.
The timings are emulator estimates, good for comparing the two.

Full tables: `build/tcl67/measure/report.txt` and
`build/measure/report.txt`.

### 7.1 Sizes and memory (`-os`, 32 KB stack)

| KB | Tcl 6.7 DOS port | Tiny Tcl 6.8.1 | Difference |
|---|---|---|---|
| `TCL.EXE` | 152.1 | 139.8 | +12.3 |
| Load image | 179.9 | 168.5 | +11.4 |
| Code: Tcl and tcl16 | 83.0 | 79.0 | +4.0 |
| Code: Open Watcom runtime | 56.5 | 51.6 | +4.9 |
| DGROUP data (without the stack) | 8.4 | 5.9 | +2.5 |
| Conventional memory at start | 192.7 | 183.9 | +8.8 |
| ... after the 30,000-byte string copy | 438.2 | 429.3 | +8.9 |

- **Tcl code.** The port has `exec`, `time`, `history` and the Unix
  error-message table (`tclUnixStr.c`); Tiny Tcl has TclX's commands.
  The port is 4 KB larger.
- **Runtime.** The port's linker map adds, among others, Open Watcom's
  `system`, the `spawn` family and its environment and path helpers
  (`cenvarg`, `dospawn`, `makepath`, `_slashc`), multibyte string
  functions they need, `mktemp`, `unlink`, `strstr` and `stricmp`. A
  tcl16 `exec` written for INT 21h/4Bh would replace most of this.
- **Growth is the same.** Lists, arrays and strings take the same
  memory in both, and neither returns freed memory to DOS (M1-REPORT,
  section 7.2).

The other variants differ in the same way: `-ox` is 152.3 KB against
139.6 KB, and `-od` 201.9 KB against 188.2 KB.

### 7.2 Stack

Identical to the KB, as the code is the same:

- 31,573 bytes are free at start;
- plain recursion reaches 53 levels at `-os`;
- recursion inside `expr` reaches 15;
- each simple level takes 954 bytes.

The 16 KB stack (`stk16`) gives 23 and 6 levels. There, `fib15` runs
out of stack and the guard (patch 0004) stops it with an error, in both.
The port's `-od` build also stops at `fib15`, as Tiny Tcl's does
(M1-REPORT, section 7.5).

### 7.3 Speed

BIOS ticks at `-os`, in the same session:

| Benchmark | Tcl 6.7 DOS port | Tiny Tcl 6.8.1 |
|---|---|---|
| `loop1000` | 278 | 279 |
| `expr1000` | 605 | 606 |
| `fexpr1000` (8087 emulated) | 753 | 754 |
| `call500` | 172 | 173 |
| `fib15` | 1383 | 1385 |
| `lappend2000` | 814 | 815 |
| `lsort500` | 256 | 256 |
| `string500` | 228 | 228 |
| `regexp200` | 118 | 119 |
| `format500` | 315 | 315 |

They are the same within a tick. `-ox` is about 15% faster for both,
and without an FPU (`os+nofpu`), `fexpr1000` takes 1239 ticks in the
port and 1240 in Tiny Tcl.

Tiny Tcl's figures moved slightly from M1's, for example `TCL.EXE` at
139.8 KB against 139.5 KB, because of patch 0006.

## 8. Tests

| Suite | Tiny Tcl (`make test`, `make test-suite`) | Tcl 6.7 DOS port (`make test-tcl67`, `make test-suite-tcl67`) |
|---|---|---|
| `tests/lang` (12), `tests/dos` (9): shared | 20 passed, 1 todo (`dospath`) | 20 passed, 1 todo (`dospath`) |
| Candidate cases: `tests/tinytcl` (6), `tests/tcl67` (7) | 5 passed, 1 todo (`crlf`) | 7 passed |
| Tcl 6.7's own suite, 42 files | 35 pass; 7 stop (no `exec`); `info-13.2` | 35 pass; 7 stop (`exec` under DOSBox-X); `info-13.2` |
| Host reference (`make test-host`) | 11 passed, 1 skipped | (shared core; not built separately) |

**Shared tests made candidate-neutral.** M1's tests assumed some Tiny
Tcl behaviour. M2 moved those expectations into `tests/tinytcl`, and
the shared cases now check what both should do:

- `loop` and `readdir`;
- sourcing a script of 32 KB or more;
- the case of `glob` results;
- the separator of `pwd`.

**Tcl 6.7's own suite.** This is the port's `TESTS/`: Tcl 6.7's 1993
tests, renamed for DOS. `scripts/run-tcl-suite` runs each file in its
own DOSBox-X session and lists the failed tests. Each candidate has a
baseline in `tests/suite/`, and a failure outside it fails the build.
The suite's files have CRLF line ends and some end with ^Z, so the
runner converts them to LF text. Otherwise Tiny Tcl fails 41 of 42
files on the file format alone (section 6).

With that, the two candidates give exactly the same results, as they
share the core:

- 35 files pass entirely, covering the parser, `expr`, lists, strings,
  `regexp`, `scan`, `format`, `trace`, `upvar`, `uplevel`, procs,
  `history`, `env` and more.
- 7 files stop at the `exec cat`/`exec pwd` that makes their fixtures:
  `cd`, `exec`, `file`, `glob`, `info`, `open` and `source`. Tiny Tcl
  has no `exec`, and the port's fails under DOSBox-X (section 6).
- `info-13.2` fails only because the runner names the file in upper
  case.

Before patch 0006, both also failed `scan-1.21` to `1.24`.

## 9. libc16 build experiment

`make libc16-try-tcl67` runs M1's experiment (M1-REPORT.md, section 6)
on the port. `build/tcl67/libc16-try/report.txt` has the report.

- **Compiling.** The 16 core modules, `src/main.c` and `src/stack.c`
  compile unchanged with libc16 0.04's headers. The 7 DOS-layer modules
  want `<malloc.h>`, and the port takes `DIR`/`opendir` from Open
  Watcom's `<direct.h>`. With declaration-only shims, everything
  compiles. The experiment now force-includes the `dirent.h` shim and
  adds a `malloc.h` shim.
- **Linking** needs everything Tiny Tcl needs:
  - the `printf` family (`sprintf` is in 16 modules);
  - `strtod`, `strtol` and `sscanf`;
  - floating point (8087 fixups, `__CHP`);
  - `opendir`/`readdir`/`closedir`, `fstat`, `fseek`/`ftell`,
    `getcwd`/`chdir`, `clearerr`;
  - `qsort`, `strcspn`, `isascii`, `abort`, `environ`.

  The port's extra commands add four more:
  - `system`, `_mktemp` and `unlink` for `exec`;
  - `strstr` for `history`.

As in M1, nothing was added to libc16. `exec` through `system()` would
need a COMMAND.COM-spawning `system` in libc16, or, better, a
tcl16-local `exec` using INT 21h/4Bh directly, which M5 is to design.

## 10. Compatibility notes

The language is Tcl 6.7, the same as Tiny Tcl's (M1-REPORT.md, section
9). The differences between the two are those of section 6, plus:

- **`history`.** The port has Tcl 6.7's `history`, with `!` and `^`
  substitution, in interactive use: `src/main.c` records commands, as
  upstream's `tclTest.c` did.
- **`exec`** follows COMMAND.COM's syntax for redirection, not Tcl's
  (`2>`, `>&` and `@` are not supported). Its words are joined with
  spaces, so quoting is the shell's.
- **Error messages** for a pipeline (`open "|..."`) and for `exec` are
  the port's own, for example "open command with pipelining no
  supported in msdos vers.".
- **`file dirname`, `file tail`, `glob`** follow `/`, and lower case.

## 11. Assessment

This is evidence for M3, not a choice.

**For the port.**

- It is stock Tcl 6.7, with the full Tcl 6.7 command set including
  `exec`, `time` and `history`, and nothing beyond it.
- It reads DOS text scripts (CRLF, ^Z) correctly.
- It passes Tcl 6.7's own suite as well as Tiny Tcl does, once both
  have the same six fixes.
- Its DOS layer is small, about 870 lines of its own.

**Against.**

- **Licensing.** Sprenger's files have no license. `exec`, `open`,
  `time` and `~` expansion would need his permission or a rewrite.
  Everything else is the University of California's code, as in Tiny
  Tcl.
- **The DOS layer is weak where it matters most.** `exec` has unchecked
  buffers, trusts the shell's status and cannot detect a missing
  program, and none of it can be tested under DOSBox-X. The design
  calls for child processes and memory recovery (DESIGN.md, sections 8
  and 12), and this `exec` would have to be rewritten for that anyway.
- **Scripts are capped at 32 KB**, where Tiny Tcl reads larger ones.
- **No TclX extras.** Tiny Tcl's `loop` and `cmdtrace` are small, but
  they are there.

**What the two candidates share.** They share the interpreter, and with
it every limit M1 measured: 32 KB strings, recursion of about 15 levels
inside `expr`, and 16-bit integer arguments. They also share six
patches and the whole libc16 gap. M3's choice is between two DOS
layers and two provenance stories, and both DOS layers fall short of
DESIGN.md's commands (`exec` with real status, DOS paths,
case-insensitive glob). A third option, Tcl 6.7's core with a DOS layer
written for tcl16, is worth weighing in M3.

## Reproducing

    make tcl67                  # fetches and checks the port, builds it
    make test-tcl67             # section 8
    make test-suite-tcl67       # Tcl 6.7's suite with the port
    make test-suite             # and with Tiny Tcl
    make libc16-try-tcl67       # section 9
    make measure-tcl67 measure  # section 7: build/tcl67/measure, build/measure
