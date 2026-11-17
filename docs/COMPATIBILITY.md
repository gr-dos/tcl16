# Tcl16 compatibility

What Tcl16's `TCL.EXE` is compatible with, where it differs, and the
limits of a 16-bit real-mode DOS program. This file starts in M4 from
[M1-REPORT.md](M1-REPORT.md), section 9, and is kept current through M7
([DESIGN.md](DESIGN.md), section 11). Each statement here is checked by
a test named beside it, or is marked as not yet specified.

## 1. The language

Tcl16 is **Tcl 6.7**, as implemented by **Tiny Tcl 6.8.1**
([ADR-001](ADR-001-tcl-implementation.md)), with Tcl16's patches
(`patches/`). `info tclversion` is `6.7`. It is not Tcl 7 or Tcl 8, and
not a subset of either.

Its 60 commands (M1-REPORT.md, section 3.2):

- **Tcl 6.7 core (42):** `append array break case catch concat continue
  error eval expr for foreach format global glob if incr info join
  lappend lindex linsert list llength lrange lreplace lsearch lsort proc
  regexp regsub rename return scan set split string trace unset uplevel
  upvar while`
- **I/O (14):** `cd close eof exit file flush gets open puts pwd read
  seek source tell`
- **TclX 6 (4):** `cmdtrace infox loop readdir`

The tests in `tests/lang` run the same on DOS and with Tiny Tcl built for
a 32-bit host (`make test-host`); Tcl 6.7's own test suite runs against
`tests/suite/tinytcl.baseline` (`make test-suite`).

### 1.1 Scripts written for Tcl 7 or Tcl 8

These need changes (M1-REPORT.md, section 9; `tests/lang`):

- `expr` takes exactly one argument. Brace it: `expr {$a + 1}`.
- There are no math functions (`int()`, `sqrt()`…). Integer division
  truncates toward zero (`-7/2` is -3), as C does.
- There are no `\x` or `\u` escapes, and no `switch`, `time`, `exec`,
  `pid`, `history`, `clock`, `file delete`, `file mkdir`, `namespace`,
  `dict` or `lassign`. `case` takes the place of `switch`. A command
  name containing `::` is just a name: there are no namespaces.
- `string range` and `lrange` report errors in their own words.
- `return`, `break` and `continue` at the top level of a script behave
  as in Tcl 6.7: `return` ends the script; `break` and `continue` are
  errors (`tests/tcl16/topbreak`).

`exec` and `file delete`/`mkdir`/`rename` are planned for M5
(DESIGN.md, section 11).

## 2. Real-mode limits

Tcl16 runs in 16-bit real mode on an 8086: an `int` has 16 bits, and no
object can be larger than a 64 KB segment.

| | Limit | Test |
|---|---|---|
| Integers | 32-bit in `expr`, `incr`, `format` and `scan`; 16-bit (-32768 to 32767) wherever else a command takes an integer, such as `lindex` or `string range`, which report larger values as errors | `tests/lang/int16`, `ints` |
| A string value, a list's text, the interpreter result | 32,767 bytes | `tests/lang/bigdata`, `tests/dos/toolong` |
| One list element, quoted with braces or backslashes | about 16 KB (quoting may double it) | M1-REPORT.md, 7.4 |
| All the words of one command, after substitution | 32,767 bytes | `tests/lang/bigdata` |
| One command in a script, including a `proc` body | 32,767 bytes | M1-REPORT.md, 7.4 |
| A script (`TCL SCRIPT`, `source`) | any size (section 3.2) | `tests/tinytcl/bigsrc`, `tests/tcl16/bigcrlf` |
| Recursion | 48 KB of stack: about 85 levels of a simple recursive proc, and about 23 when each level recurses inside `expr` (DESIGN.md, section 12) | `tests/dos/deep`, `tests/tcl16/stack`, `replstk` |

Past a size limit, Tcl16 stops with a message ("tcl16: string or list
too long", or "tcl16: out of memory") and exit status 255, after
flushing standard output; it does not corrupt memory. Past the stack
limit, the command fails with the Tcl error "too many nested calls to
Tcl_Eval (out of stack)", which `catch` can trap.

Memory: `TCL.EXE` takes about 186 KB of conventional memory at start,
and needs about 430 KB for the heaviest workload measured
(M1-REPORT.md, 7.2). Its heap never returns memory to DOS, so `unset`
frees memory for Tcl's reuse, not for DOS.

## 3. DOS text

### 3.1 Scripts and the REPL

Scripts (`TCL SCRIPT`, `source`) and the commands typed at the REPL are
read as **DOS text** (`src/dostext.c`):

- **CR LF** ends a line, as LF does. So a backslash before CR LF
  continues the line, and a quoted or braced value spanning lines has
  LF, not CR LF, between them. A CR that is not before an LF is kept.
- **^Z** (1Ah), which DOS editors and `COPY /A` put at the end of a text
  file and which the console sends for Ctrl-Z, ends the text: nothing
  after it is read.

Tests: `tests/tinytcl/crlf`, `ctrlz`; `tests/tcl16/dosmain` (a script
that is itself DOS text), `bigcrlf`, `repldos`.

Tiny Tcl itself read scripts in binary mode, so these were bugs before
M4 (DESIGN.md, section 10.1).

### 3.2 Scripts and commands

A script of up to 32,766 bytes is read whole and evaluated as one Tcl
script, as in Tcl 6.7. A larger one, which no Tcl string could hold, is
read line by line, and each command is evaluated as soon as it is
complete; only each **command** must then fit the limits of section 2.
The two behave the same: `return` ends the script and becomes the
result of `source`, and an error's line in errorInfo is counted from the
start of the file (`tests/tcl16/bigret`). A command of more than 32,765
bytes in such a script is reported as "command too long".

### 3.3 Files

`open`, `gets`, `read` and `puts` use the C runtime's text mode: `puts`
writes `\n` as CR LF, and `gets` and `read` return LF for CR LF
(`tests/dos/files`). How they treat ^Z inside a file is not yet
specified (M5). Error messages from the C library, such as "No such file
or directory", are Open Watcom's text in the development build and may
change with libc16 (M7).

## 4. The `TCL.EXE` command line

`TCL /?` prints this summary (`tests/tcl16/usage`):

```text
TCL                     interactive; end with EXIT or Ctrl-Z
TCL script [arg ...]    run a script; argv0, argv and argc are set
TCL -e text [arg ...]   run the Tcl commands in text (argv0 is -e)
TCL -- script [arg ...] run a script whose name starts with -
TCL /?                  this help
```

Tcl 6.7 itself has no standard shell; these conventions are Tcl16's
own. **Tcl16 extensions**, not historical Tcl features: `-e`, `--`,
`/?` (and `-?`, `-h`), the `> ` continuation prompt, and the exit
statuses below.

### 4.1 Script mode

`TCL SCRIPT A B` sets `argv0` to `SCRIPT` as typed, `argv` to the list
`A B` and `argc` to 2, as Tcl 6.7's applications did
(`tests/dos/argv`). The DOS command line is at most 126 characters; the
C runtime splits it into arguments at blanks, and `"..."` makes one
argument with blanks in it.

`TCL -e TEXT A B` evaluates TEXT with `argv0` set to `-e` and `argv` to
`A B` (`tests/tcl16/eopt`). `TCL -- SCRIPT` runs a script whose name
starts with `-` (`tests/tcl16/ddash`). Any other argument that starts
with `-` is an unknown option. An argument that starts with `/` other
than `/?` is a script name, so `TCL /SCRIPTS/A.TCL` works.

### 4.2 Interactive mode

`TCL` with no arguments reads commands from standard input
(`tests/tcl16/repl`):

- It prompts with `% `, and with `> ` while a command is incomplete
  (an open brace, quote or bracket).
- It prints each command's result, if not empty, on standard output, and
  an error as `Error: MESSAGE` on standard error, then goes on.
  `return` at the top level gives its value as the result.
- It ends at the end of its input: Ctrl-Z (then Enter) on the console,
  or the end of a redirected file. A command still incomplete then is
  not run: "Error: incomplete command at end of input", exit status 1
  (`tests/tcl16/replinc`). `exit` ends it at once (`replexit`).
- It prompts even when standard input is redirected, as Tcl 6.7's
  `tclTest` did. To run a script, give its name: `TCL SCRIPT.TCL`.
- There is no line editing beyond what DOS's console input gives, and no
  command history (`history` is not in Tiny Tcl).

`argv0` is the path of `TCL.EXE`, `argv` is empty and `argc` is 0.

### 4.3 Startup

`TCL.EXE` prints no banner, sources no startup script and sets no
`tcl_library`. Tiny Tcl's own driver printed a banner and sourced
`tcl_sys/autoinit.tcl`, whose procedures need Tiny Tcl's Borland-only
DOS commands, which Tcl16 does not build (DESIGN.md, section 8). `info
library` reports the `TCL_LIBRARY` environment variable, or the error
"there is no Tcl library at this installation", as in Tcl 6.7
(`tests/tcl16/startup`).

### 4.4 Diagnostics and exit status

Diagnostics go to standard error, after standard output is flushed, so
the two keep their order when both reach the screen or the same file.
COMMAND.COM cannot redirect standard error.

| Exit status | When | Test |
|---|---|---|
| 0 | The script, `-e` text or REPL ended normally | `tests/tcl16/repl` |
| N | `exit N`; DOS keeps N modulo 256 | `tests/dos/exit`, `tests/tcl16/eexit`, `replexit` |
| 1 | An uncaught error in a script or `-e` text: `Error: ` and errorInfo, which says where, on standard error; also a script that cannot be read, `break` or `continue` outside a loop, and a command left incomplete at the end of the REPL's input | `tests/tcl16/uncaught`, `eerror`, `missing`, `topbreak`, `replinc`; `tests/dos/errexit` |
| 2 | A bad command line: `TCL: unknown option -x` or `TCL: missing argument after -e`, and `Type TCL /? for help.` | `tests/tcl16/badopt`, `emissing` |
| 255 | Out of memory, or a string or list too long (section 2) | `tests/dos/toolong` |

## 5. Other differences

- Tiny Tcl's `putenv` replaces the C library's, so `env` changes will
  reach child programs once there are any (M5).
- DOS paths: `pwd` uses `\`; `file dirname`, `tail` and `rootname`
  split only at `/`, and `glob` matches case-sensitively against the
  upper-case names DOS stores. Both are known failures due in M5
  (DESIGN.md, section 10.1; `tests/dos/dospath`, `tests/tinytcl/glob`).
- Floating point in `expr`, `format` and `scan` uses an 8087 if there
  is one, and Open Watcom's emulator if not, in the development build.
  Its future in the libc16 build is open (DESIGN.md, section 12).
