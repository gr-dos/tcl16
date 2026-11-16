# ADR-001: Tiny Tcl 6.8.1 as the interpreter

- **Status:** Accepted (M3)
- **Milestone:** M3 of [DESIGN.md](DESIGN.md), section 11
- **Evidence:** [M1-REPORT.md](M1-REPORT.md) (Tiny Tcl 6.8.1),
  [M2-REPORT.md](M2-REPORT.md) (Tcl 6.7 DOS port),
  [PROVENANCE.md](PROVENANCE.md); pull requests #1 and #2

## Context

DESIGN.md named two candidate source bases for Tcl16 and asked for both
to be evaluated before one was chosen (section 2). M1 and M2 did that
with the same toolchain, patches, tests, measurements and libc16
experiment:

- **Tiny Tcl 6.8.1** (2001): Tcl 6.7 with TclX 6 pieces and a DOS I/O
  layer, under the Tcl license. Vendored unmodified in
  `vendor/tinytcl-6.8.1`.
- **Peter Sprenger's Tcl 6.7 DOS port** (1993): stock Tcl 6.7 with
  `msdos.c`, a reworked Unix layer and `exec` through COMMAND.COM. It is
  fetched and checked by hash, not vendored, because its own files carry
  no license.

The evaluations found the two are the same interpreter. Tiny Tcl's core
is Tcl 6.7, and most core files differ from the release only in RCS
headers. Both needed the same six patches to run safely with a 16-bit
`int` and Open Watcom's register calling convention. With those
patches, both give identical results on every file of Tcl 6.7's own test
suite that both can run. They also have the same speed and the same
recursion and object-size limits. The choice was therefore between two
DOS layers, two footprints and two provenance stories.

## Decision

**Tcl16 is built on Tiny Tcl 6.8.1.** Tcl16 is a Tcl 6.7-compatible
interpreter for 16-bit real-mode DOS, based on Tiny Tcl 6.8.1. It does
not aim at Tcl 7 or Tcl 8 compatibility, and its documentation must not
imply it.

The Tcl 6.7 DOS port is not a foundation for Tcl16. It stays as a
reference: its M2 report, patches (`patches/tcl67/`), tests
(`tests/tcl67/`), suite baseline and `make` targets are kept as they
are. Tcl16 does not commit to maintaining them alongside Tiny Tcl, and
a later milestone may retire them if keeping them costs more than they
show.

## Rationale

1. **The cores are equivalent.** Same Tcl 6.7 interpreter, same defects
   and fixes (patches 0002 to 0006), same suite results, same speed
   within one BIOS tick, same stack depth and the same 32 KB object
   limit (M2-REPORT.md, sections 1, 7 and 8). Neither candidate offers a
   better language.
2. **Tiny Tcl is smaller.** At `-os` with a 32 KB stack, its `TCL.EXE`
   is 139.8 KB against 152.1 KB (12.3 KB smaller). It takes 183.9 KB of
   conventional memory at start against 192.7 KB (8.8 KB less)
   (M2-REPORT.md, section 7.1).
3. **Tiny Tcl does more where it matters.** It adds TclX's `loop`,
   `cmdtrace`, `readdir` and `infox`, and it `source`s scripts of 32 KB
   or more command by command. The port stops with an error on such
   scripts (M2-REPORT.md, section 6).
4. **Tiny Tcl's provenance and license are clear.** It is vendored byte
   for byte, checked against the release archive, and under the Tcl
   license (`vendor/LICENSE.tinytcl`). The port's `msdos.c` and
   `msdos.h` have no license. Its archive includes Borland's
   copyrighted `errno.h` (`TCLERRNO.H`) and third-party files of
   unknown terms (M2-REPORT.md, section 2; PROVENANCE.md).
5. **The port's DOS advantages do not carry over.** The port has the
   only `exec`, and it reads CRLF and ^Z scripts correctly. But its
   `exec` uses unchecked fixed buffers and trusts the shell's exit
   status. It cannot detect a missing program, gives a child the
   keyboard as input and does nothing to recover memory. Whether it
   works under DOSBox-X depends on the version. Its paths split only at
   `/`, its `glob` is case-sensitive, and it uses 129-byte static path
   buffers (M2-REPORT.md, sections 1 and 6). DESIGN.md's `exec`, path
   and glob behaviour would have to be written again either way, and
   the code that would be replaced is the unlicensed code. Text-mode
   `source` is a small fix for Tcl16 to make in Tiny Tcl.

## Alternatives considered

- **The Tcl 6.7 DOS port.** Rejected for the reasons above: an
  equivalent core, a larger footprint, unclear licensing, and a DOS
  layer that would be replaced.
- **Tcl 6.7's release core with a DOS layer written for Tcl16**
  (raised in M2-REPORT.md, section 11). Tiny Tcl already is the Tcl 6.7
  core with an interchangeable DOS layer. Its DOS layer is permissively
  licensed and works today. Starting from the bare release would give
  up TclX, large-script `source` and that working layer for no gain in
  the core. The decision keeps the useful part of this idea: the DOS
  layer is Tcl16's to own and replace (DESIGN.md, section 8).
- **Carrying both interpreters.** Rejected by DESIGN.md, section 2.1
  ("choose one"). There is no compelling reason. The candidates differ
  only in parts Tcl16 will rewrite.
- **A newer Tcl (7.x or 8.x).** Not evaluated, and outside DESIGN.md's
  candidates. Later Tcl releases target 32-bit systems and are much
  larger. Tcl 6.7 already needed six patches to run safely in 16 bits.

## Consequences

- **Language.** Tcl16's language is Tcl 6.7, with TclX's `loop`,
  `cmdtrace`, `readdir` and `infox`. It has no `switch`, `time`,
  `history`, namespaces, `dict` or Tcl 8 features. `expr` takes one
  argument. M1-REPORT.md, section 9, lists the differences scripts meet,
  and DESIGN.md, section 7, records the baseline.
- **Inherited limits stay until later milestones address them.** These
  are the 32,767-byte string and list limit, about 15 levels of
  recursion inside `expr` with a 32 KB stack, and 16-bit integer
  arguments outside `expr`, `incr` and `format`. The heap also never
  shrinks (M1-REPORT.md, section 7).
- **Upstream stays pristine.** `vendor/tinytcl-6.8.1` is not edited.
  Changes to Tiny Tcl's files are tracked patches in `patches/`, each
  with its explanation.
- **Tcl16 owns the DOS layer.** This covers `exec`, DOS paths,
  case-insensitive filename matching, text-mode scripts, the command
  line and DOS/GR-DOS introspection. Tiny Tcl's `tcldos*.c` files are
  where it starts. Tiny Tcl's Borland-specific `dos.c` is not built and
  is not a basis for these commands (DESIGN.md, section 8).
- **The `exec` work is new.** Tiny Tcl has none. M5 designs it,
  including memory recovery before a child runs. Until then, the 7 files
  of Tcl 6.7's suite that need `exec` for their fixtures do not run with
  Tiny Tcl.
- **libc16 work is unchanged by the choice.** Both candidates need the
  same functions (M1-REPORT.md, section 6), and the clean-room rule of
  DESIGN.md, section 4, applies to every addition.
- **Known failures are debt.** The `todo` cases (`tests/dos/dospath`,
  `tests/tinytcl/crlf`) and the entries in `tests/suite/tinytcl.baseline`
  record behaviour that is wrong today. They are not accepted behaviour.
  The milestones in DESIGN.md, section 11, are expected to remove them.

## Open engineering decisions

These follow from the choice but are not made here. DESIGN.md, section
12, tracks them and names the milestone for each:

- conventional-memory recovery when spawning a child program;
- a safe `exec` over INT 21h/4Bh: bounded command tails, real exit
  status, missing-program errors, input and output redirection;
- the stack size, and whether to move large stack buffers to the heap;
  the 32 KB object limit, and whether a stop past it can become a Tcl
  error;
- floating point: an 8087 emulator, a clean-room software
  implementation, or integer-only `expr`;
- minimal formatting for Tcl's internal `sprintf` uses and the `format`
  command, without a mandatory `libkitchen16`;
- the remaining libc16 gaps and which of them belong in libc16 rather
  than in Tcl16-local adapters;
- DOS and GR-DOS introspection, using the GR-DOS kernel identification
  API of gr-dos/gr-dos#46 when it exists.
