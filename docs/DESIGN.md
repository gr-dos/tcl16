# Tcl16 Design

**Project:** `gr-dos/tcl16`  
**Status:** M1 and M2 evaluated; M3 interpreter selection complete ([ADR-001](ADR-001-tcl-implementation.md)); M4 `TCL.EXE`, M5 DOS integration and M6 DOS/GR-DOS introspection complete; M7 next  
**Target:** Real-mode, 16-bit DOS, including GR-DOS and compatible DOS implementations

## 1. Purpose

Tcl16 is a native 16-bit DOS Tcl interpreter that makes Tcl scripting useful on vintage PCs, including 8086-class machines. It will be a standalone `TCL.EXE`, not a required component of the GR-DOS kernel or command processor.

**Tcl16 is a Tcl 6.7-compatible interpreter for 16-bit real-mode DOS, based on Tiny Tcl 6.8.1.** It does not aim at Tcl 7 or Tcl 8 compatibility.

The primary goals are a useful, reasonably Tcl-compatible scripting environment, a manageable conventional-memory footprint, and compatibility with GR-DOS **and other DOS kernels**.

## 2. Interpreter

The interpreter is **Tiny Tcl 6.8.1**: the Tcl 6.7 core with TclX 6 pieces and a DOS I/O layer, under the Tcl license. [ADR-001](ADR-001-tcl-implementation.md) records the decision, the alternatives and the evidence. The selection is closed. Later milestones build on it and do not reopen it.

### 2.1 How it was chosen

DESIGN.md originally named two candidates and set an evaluation process for each: pin the source, audit it, build it with Open Watcom, run common tests, measure it, and choose one. M1 and M2 carried that out:

| Candidate | Report | Outcome |
| --- | --- | --- |
| **Tiny Tcl 6.8.1** | [M1-REPORT.md](M1-REPORT.md) | **Selected.** Smaller (139.8 KB `TCL.EXE`, 183.9 KB at start), TclX extras, `source` of scripts of 32 KB or more, clear provenance and license |
| **Historical Tcl 6.7 DOS port** (Peter Sprenger, 1993) | [M2-REPORT.md](M2-REPORT.md) | Not selected. Same core and the same results, but 12 KB larger, with unlicensed DOS files and an `exec` that would have to be rewritten |

The two share the Tcl 6.7 interpreter core, its six 16-bit patches, its test results, speed and limits. They differ only in the DOS layer, footprint and provenance, which Tcl16's own DOS layer (section 8) takes over in any case.

### 2.2 Historical reference material

The M2 port's evaluation is kept as reference, not as a second product: `docs/M2-REPORT.md`, `patches/tcl67/`, `tests/tcl67/`, `tests/suite/tcl67.baseline`, `scripts/fetch-tcl67` and the `*-tcl67` `make` targets. They document behaviour (for example, text-mode `source` and a COMMAND.COM `exec`) that Tcl16's own work can compare against. The port's fetched archive also supplies Tcl 6.7's own test suite, which `make test-suite` runs against Tiny Tcl. Retiring the port must keep that suite available, for example from the Tcl 6.7 release archive. Tcl16 does not promise to keep them building indefinitely. A later milestone may retire them when they stop being useful, and must say so when it does.

## 3. Platform and build requirements

- **CPU:** 8086/8088 minimum; no required 286/386 instructions.
- **Execution:** 16-bit DOS real mode. No required DOS extender, DPMI host, or protected-mode runtime.
- **DOS:** Aim for DOS 2.0 and later, subject to verifying actual APIs and utility dependencies. Run on GR-DOS and other compatible kernels.
- **Compiler:** Open Watcom C, following the established GR-DOS toolchain practices.
- **C runtime:** `libc16` (section 4). The development build links Open Watcom's runtime, because libc16 lacks functions Tiny Tcl needs: about twenty in 0.04 (M1-REPORT.md, section 6), fewer since 0.06 (section 4.2). Closing that gap is planned work (sections 11 and 12), not a reason to keep Open Watcom's runtime.
- **Memory model:** Large. M1 found no object needs the huge model, because sizes stop at 32 KB. Change only for measured technical reasons.
- **Optional libraries:** Do not require `libkitchen16` or a full C `printf` merely to provide basic Tcl functionality.
- **Build reproducibility:** Pin tool versions and dependencies where practical; provide clear build instructions, deterministic artifact conventions, and `make help` consistent with other GR-DOS projects.

### 3.1 Memory and failure behavior

An 8086's segmented addressing introduces near/far-pointer distinctions and 64 KiB limits on individual segments and objects. Large-model compilation does **not** eliminate these limits or repair unsafe assumptions about pointer arithmetic.

M1 audited Tiny Tcl for these issues and fixed what it found (patches 0002 to 0006; M1-REPORT.md, sections 5 and 7). The limits that remain:

- A string value, a list's text or the interpreter result holds at most 32,767 bytes; one list element about 16 KB; all the words of one command together 32,767 bytes.
- With `TCL.EXE`'s 48 KB stack (M4), recursion reaches about 85 levels of a simple proc, and about 23 levels when each level recurses inside `expr`.
- Integer arguments are 16-bit, except in `expr`, `incr`, `format` and `scan`.
- The heap never returns memory to DOS.

Past the object and stack limits, the interpreter either reports a Tcl error (stack) or stops cleanly with a message (sizes, out of memory). It does not corrupt memory at any site found so far. The audit of `int`-sized lengths was driven by tests and is not exhaustive. Further work continues to look for:

- Near/far pointer validity, comparisons, and casts.
- Object lengths and offsets that could exceed 16-bit ranges.
- Stack use, interpreter nesting, and recursion.
- Small-allocation fragmentation and heap exhaustion.
- Memory retained while invoking child DOS programs.

Allocation failures must report a useful error where recovery is possible; they must not silently corrupt interpreter state. State any documented limits rather than suggesting support for arbitrarily large objects.

## 4. Components and ownership

Tcl16 is built from four components with distinct owners and rules.

| Component | Contents | Rules |
| --- | --- | --- |
| **Tiny Tcl upstream** | The Tcl 6.7 interpreter core, Tiny Tcl's TclX pieces, and its suitably licensed DOS I/O commands, in `vendor/tinytcl-6.8.1/` | Vendored unmodified and checked against its manifest (`make check-vendor`). Every change is a tracked, explained patch in `patches/` (PROVENANCE.md). 16-bit compatibility fixes belong here. Tcl16 does not build upstream files without a clear license notice as a basis for new features (for example, `dos.c`) |
| **Tcl16 DOS layer** | The CLI and REPL (`TCL.EXE`), DOS text handling (CRLF and ^Z), drive-letter and backslash path semantics, case-insensitive filename matching, filesystem and process execution, DOS/GR-DOS introspection where available | Owned by Tcl16. New code lives in `src/` as Tcl16's own work. Where the behaviour is in an upstream file (`tcldos*.c`, `tclEnv.c`), Tcl16 either patches it or replaces it with its own source. Either way, the behaviour is Tcl16's responsibility and must be tested |
| **`libc16`** | The shared minimal C runtime | Used through its own headers and declarations. Additions only when generally useful to 16-bit C programs, and **always clean-room** (section 4.1) |
| **`libkitchen16`** | Optional shared heavier functionality | Used only where measurement shows a benefit. Never a mandatory dependency merely to obtain a full `printf` (section 5) |

Tcl-specific semantics (command behaviour, Tcl error messages, Tcl's `format`) stay inside Tcl16, never in the C library.

### 4.1 `libc16` and the clean-room policy

`libc16` is the minimal common C runtime. Tcl16 should use its existing interfaces wherever possible, and **must not expand `libc16` solely for convenience** if a small Tcl16-local adapter is more appropriate.

#### Mandatory clean-room requirement

**Any additions or changes to `libc16` made to support Tcl16 must be clean-room implemented.** Implement them independently from documented behavior, public interface specifications, and original test cases; **do not copy, translate, or mechanically adapt source from Tcl or Tiny Tcl, the historical Tcl 6.7 DOS port, Open Watcom's or other DOS runtime libraries, other libc implementations, or other third-party C code into `libc16`.** When reviewing upstream source to identify required behavior, maintain a clear separation between requirements and independent implementation. Record the specification basis, authorship, and tests for any new `libc16` facility.

Any legitimately reused or vendored interpreter source belongs in Tcl16 under its own applicable license, not silently incorporated into `libc16` as though it were original code. A dependency's license being permissive does **not** waive the project's clean-room requirement for `libc16`.

Additional rules:

- Add a facility to `libc16` only if it is generally useful to 16-bit C applications and fits that library's minimal-runtime scope.
- Prefer separate reusable components, including `libkitchen16` for heavier optional functionality, over increasing every application's baseline footprint.
- Verify the existing `libc16` API before proposing an addition (e.g., allocation, stdio, environment, execution, directory enumeration).
- Use only libc16's own declarations for anything libc16 provides. Upstream's private declarations compile to a different calling convention (M1-REPORT.md, section 6).
- Supply focused tests under GR-DOS and at least one other compatible DOS environment for each changed runtime facility.

### 4.2 The libc16 gap

M1 measured the gap against libc16 0.04 (M1-REPORT.md, section 6). libc16 0.05 to 0.15 closed most of it, and **since M5 `TCL.EXE` links libc16 0.15 and libkitchen16 0.02** (`scripts/fetch-libc16`, `scripts/fetch-libkitchen16`), with no Open Watcom runtime. How each need of M1's list is met:

| Need | Functions | Met by |
| --- | --- | --- |
| Conversion, strings, sorting, files, termination | `strtol`, `strcspn`, `strstr`, `isascii`, `qsort`, `fstat`, `fseek`, `ftell`, `getcwd`, `chdir`, `clearerr`, `abort` | libc16 (0.05, 0.06); `qsort`'s comparator is `__cdecl` (patch 0007) |
| Formatted output | `sprintf`, `fprintf`, `vfprintf` | libkitchen16 (section 5) |
| Floating conversion | `strtod` | libkitchen16 |
| Floating point | the `-fpc` helper calls | libkitchen16's software floating point (section 12) |
| Formatted input | `sscanf` (`scan`) | Tcl16's `src/scan.c` (`Tcl16_Sscanf`, patch 0010), the conversions `scan` documents |
| Directories | `opendir`, `readdir`, `closedir` | Tcl16's `src/dosdir.c`, over libc16's `_dos_findfirst`/`_dos_findnext` |
| Environment | `environ` | Tcl16's `src/dosenv.c`, from the PSP's environment block, for `tclEnv.c` (patch 0010) |
| Child processes | `exec` | Tcl16's `src/exec.c`, over libc16's `spawnve` (INT 21h 4B00h) and `libc16_systeme` (`COMSPEC /C`) |

`panic` exits with status 255 itself, since libc16's `abort` exits with 3 (patch 0010). libc16 0.15 still lacks `fopen`'s update modes (`r+`, `w+`, `a+`), which Tiny Tcl's `open` passes through (section 10.1).

The M2 reference build of the Tcl 6.7 DOS port no longer compiles `tclCmdIL.c` against libc16 0.06 or later, for the same `qsort` reason. Tcl16 does not patch it (section 2.2).

## 5. Formatting policy

Perl16 may require a relatively comprehensive C-style formatting implementation, potentially placed in the optional `libkitchen16` library. Tcl16 should **not automatically inherit that dependency**.

Tiny Tcl has Tcl's `format` command, implemented by passing each field to C's `sprintf`. Its other uses of `sprintf` are a short list of fixed formats (`%d`, `%ld`, `%u`, `%s`, `%g`, `%.150s`). Tcl's script-level `format` is distinct from C's variadic `printf`. The preferred direction is a compact Tcl16-local formatter covering Tiny Tcl's internal formats and the `format` command's conversions. A separately linkable shared formatting engine is acceptable only if measured code size and behaviour show it is better.

Do not make `libkitchen16` mandatory without an explicit benefit demonstrated by testing; do not assume the linker removes unused functionality more granularly than the object-file layout and linker actually permit.

**Settled in M5: libkitchen16.** `TCL.EXE` takes `sprintf`, `vfprintf`, `strtod` and its floating point from libkitchen16 0.02, linked member by member like libc16. The measured benefit (`make measure`, the `os` variant, against M1-REPORT.md, 7.1 and 7.2):

| | Open Watcom runtime (M4) | libc16 and libkitchen16 (M5) |
| --- | --- | --- |
| `TCL.EXE` | 139.5 KB | 120.5 KB |
| Code of the C runtime | 51.6 KB | 22.0 KB |
| Conventional memory taken at start | 185.9 KB | 152.3 KB |

The Tcl and Tcl16 code grew from 78.7 KB to 92.1 KB in the same time, with M5's `file`, `glob`, `exec` and `scan`. A Tcl16-local formatter would have had to cover `format`'s conversions, floating point included, for no measured gain over libkitchen16's members.

## 6. User-facing executable

**Name:** `TCL.EXE`. M4 implemented this command line in `src/main.c`. [COMPATIBILITY.md](COMPATIBILITY.md), section 4, documents it as built: options, prompts, startup and exit statuses.

Proposed invocations:

```dos
C:\>TCL
% set x 10
10
% expr {$x * 2}
20

C:\>TCL SCRIPT.TCL
C:\>TCL -e "puts {Hello, DOS!}"
C:\>TCL /?
```

Required modes:

- Interactive read/evaluate/print loop with usable Tcl continuation behavior for incomplete commands.
- Script execution from a pathname; pass through script arguments in Tcl 6.7's form (`argv0`, `argv`, `argc`, as `TCL.EXE` does).
- Command-line expression/script execution via `-e` as a Tcl16-specific convenience.
- Concise `/?` usage output, DOS-friendly errors, and meaningful exit statuses.
- Scripts and `source`d files written with DOS editors (CRLF line ends, a trailing ^Z) read correctly.

Preserve standard Tcl 6.7 parsing and evaluation semantics. Clearly document any CLI options or behaviors introduced by Tcl16 rather than implying they are historical Tcl features. DOS path syntax, quoting, command-tail limits, and standard streams require explicit tests.

## 7. Supported language baseline

The language is **Tcl 6.7**, as implemented by Tiny Tcl 6.8.1, plus TclX 6's `loop`, `cmdtrace`, `readdir` and `infox`. It is not a Tcl-inspired dialect and not a subset of Tcl 8. M1 tested the baseline on an emulated 8086 (M1-REPORT.md, sections 3.2 and 8):

| Capability | Importance | Status with Tiny Tcl 6.8.1 |
| --- | --- | --- |
| Variables and substitutions; `set`, `unset` | Required | Present, tested |
| `if`, `while`, `for`, `foreach` | Required | Present, tested (plus `case`, Tcl 6's `switch`) |
| `proc`, `return`, procedure scopes | Required | Present, tested; recursion depth limited (section 3.1) |
| `expr` and common integer operators | Required | Present, tested; 32-bit integers; one argument only |
| Lists and common list operations | Required | Present, tested; 32 KB limit |
| Strings and common string operations | Required | Present, tested; 32 KB limit |
| `source` | Required | Present, tested; scripts of any size, read as DOS text (CR LF, ^Z) since M4 |
| `catch`, `error` | High | Present, tested |
| Arrays | High | Present, tested |
| `eval`, `uplevel`, `upvar` | High | Present, tested |
| `format` | Desirable; assess footprint | Present, tested; depends on C `sprintf` (section 5) |
| Floating-point expression operations | Evaluate separately | Present, tested; libkitchen16's software floating point since M5 (section 12) |
| `exec` and child programs | Required by section 8 | Present since M5, tested (`src/exec.c`; COMPATIBILITY.md, section 6) |
| `file delete`/`mkdir`/`rename` | Desirable | Present since M5, tested (`src/file.c`; COMPATIBILITY.md, section 5) |
| `time`, `history`, `pid`, `switch`, namespaces | Not planned for the baseline | Absent from Tiny Tcl; may be added only with a documented reason |
| Tk or GUI support | Out of scope | — |
| Dynamic binary extension loading | Out of scope initially | — |

Avoid advertising complete Tcl 6.7 compatibility until each command is verified, and never imply modern Tcl compatibility. Tcl 6.x predates namespaces; names containing `::` do **not** imply namespace support. Scripts written for later Tcl versions meet the differences listed in [COMPATIBILITY.md](COMPATIBILITY.md), which M4 started from M1-REPORT.md, section 9.

Example target script, which case-insensitive `glob` (M5) makes work:

```tcl
set count 0
foreach file [glob *.*] {
    puts $file
    incr count
}
puts "Total files: $count"
```

## 8. DOS integration layer

The DOS layer is Tcl16's own (section 4). Keep DOS-specific services separate from the interpreter core, so the Tcl engine remains understandable and the DOS interfaces can be tested independently. Tiny Tcl's `tcldosaz.c`, `tcldosut.c`, `tcldosst.c` and `tcldosgl.c` are the starting point for files, `glob`, `cd`, `pwd` and `source`. Tiny Tcl's Borland-specific `dos.c` is not built. Its commands are not a basis for Tcl16's: they are redesigned and rewritten here.

### 8.1 Initial facilities

- Filesystem operations, directory enumeration/globbing, and file I/O, with DOS path handling: drive letters, `\` and `/` as separators in `file dirname`/`tail`/`rootname`/`extension`, and case-insensitive filename matching in `glob`.
- DOS text: CRLF and ^Z in scripts, consistent with how `open`/`gets`/`read` already translate line ends.
- Access to process environment variables and working directory.
- Execution of external DOS programs (`exec`), written for Tcl16 over INT 21h/4Bh, with clear documentation of conventional-memory constraints. It needs bounded command tails, the child's real exit status, an error for a missing program, and explicit input and output redirection. The M2 port's `exec` shows what to avoid: unchecked buffers, shell-dependent status and console input by default (M2-REPORT.md, section 6).
- Standard input, output, and error handling, including redirection where supported.
- Generic DOS version reporting.

Distinguish standalone executables from command-processor builtins. For example, running `dir` may require `COMMAND.COM /C`, whereas launching an `.EXE` does not necessarily require a shell. Capturing child output may require temporary files or redirection rather than Unix-style pipes.

### 8.2 GR-DOS detection

Use the proposed **GR-DOS kernel identification/version API** (tracked in [`gr-dos/gr-dos#46`](https://github.com/gr-dos/gr-dos/issues/46)) when it exists. That API should distinguish the actual GR-DOS kernel from the separately reported MS-DOS compatibility version and return a GR-DOS release version in **`n.nn`** format, plus an independent identification API revision.

Tcl16 must also work on non-GR-DOS kernels. A missing identification API should yield a normal, documented "unknown / not GR-DOS" result rather than a fatal error.

The API exists since gr-dos/gr-dos#46 closed: INT 21h AX=3347h, BX=4752h, with a nonce in DX (gr-dos's `docs/grver.md`). M6 implemented these commands, as one command with options (`src/dosinfo.c`; COMPATIBILITY.md, section 7):

```tcl
dos version          ;# the MS-DOS version DOS reports, n.nn
dos kernel           ;# GR-DOS, or unknown
dos kernelversion    ;# GR-DOS's release, n.nn, or empty
dos memory           ;# the largest free block, in bytes
dos drives           ;# A: C: ...
```

**Naming, settled in M6: one `dos` command with options.** The provisional `dos::version` names would be literal names in Tcl 6.7, and would suggest namespaces it does not have; `dos_version`-style names would put five names in the global table, beside a script's own procs. One command with options is what Tcl 6.7 does itself (`file`, `info`, `string`). Options are given in full, since `kernel` is a prefix of `kernelversion`. Each option is one INT 21h call (two for the identification), with no arguments.

Recognition follows GR-DOS's `GRVER`: two calls, nonces 5A3Ch and C3A5h, each answer checked (signature, complemented nonce, revision at least 1, minor at most 99) and the two the same. Anything else is `unknown`, not an error. The API revision is not exposed: revision 1 adds nothing to the version, and its capability byte is reserved. Future feature detection should prefer explicit API capabilities over assumptions based solely on a GR-DOS release number; an option for them belongs with the first capability a script can use.

### 8.3 Deferred capabilities

Serial ports, networking, substantial command-output capture, and dynamic extension systems are not prerequisites for the initial release. Evaluate only after the core interpreter and filesystem/process model work reliably.

## 9. Source layout and dependencies

Current layout:

```text
tcl16/
├── README.md
├── Makefile                 make help lists every target
├── src/                     Tcl16's own C: main.c (TCL.EXE's command line and
│                            REPL), dostext.c and source.c (DOS text, source),
│                            stack.c (stack guard), probe.c (measurement only),
│                            tcl67main.c (M1/M2 driver, M2 reference build);
│                            the DOS layer: file.c, glob.c, exec.c (M5),
│                            dosinfo.c (M6)
├── vendor/
│   ├── tinytcl-6.8.1/       Tiny Tcl, unmodified, with its manifest
│   └── LICENSE.tinytcl
├── patches/                 0001–0009: Tcl16's patches to Tiny Tcl
│   └── tcl67/               M2 reference: patches to the Tcl 6.7 DOS port
├── scripts/                 fetching, building, measuring, test runners
├── tests/
│   ├── lang/ dos/           portable language and DOS cases
│   ├── tinytcl/             Tiny Tcl-specific cases
│   ├── tcl16/               TCL.EXE's command line, REPL and DOS text
│   ├── tcl67/               M2 reference cases
│   ├── grdos/               cases run on GR-DOS itself (make test-grdos)
│   ├── tools/               DOS test tools (FREEMEM, ERR2OUT, CHILD,
│   │                        FAKEID...)
│   ├── suite/               baselines for Tcl 6.7's own test suite
│   └── probe/               measurement scripts
└── docs/
    ├── DESIGN.md
    ├── COMPATIBILITY.md     what TCL.EXE is compatible with, and its limits
    ├── ADR-001-tcl-implementation.md
    ├── M1-REPORT.md  M2-REPORT.md
    └── PROVENANCE.md
```

Planned additions: `docs/PORTING.md` if porting notes outgrow the reports, and `examples/` (M7). `docs/COMPATIBILITY.md` (M4) is kept current through M7.

Keep upstream code and Tcl16's own code clearly separated. Respect each source's actual license (PROVENANCE.md). The license for Tcl16's own files is not yet decided and must be settled before a release (M7). The maintained product is a single self-contained DOS executable with no external dynamic runtime. Statically registered Tcl commands are the extension mechanism.

## 10. Validation

Test the Tiny Tcl-based `TCL.EXE` with the shared suites, recording precise build parameters and host/emulator/hardware configurations. The M2 port's suites remain runnable as reference (section 2.2).

**Language:** quoting/braces, substitution, nested evaluation, arithmetic, procedures, scope, loops, lists, error propagation, sourcing, arrays and formatting. Tcl 6.7's own test suite (`make test-suite`) runs against a baseline of expected failures.

**DOS behavior:** paths with drive letters and backslashes, file enumeration, open/read/write/close, CRLF and ^Z scripts, environment variables, command-line arguments, redirection, shell builtins vs executables, exit codes, and low-memory execution.

**Memory and portability:** very small free conventional memory, repeated allocations, nested procedures, 64 KiB boundaries, stack exhaustion, clean shutdown, 8086 compatibility, GR-DOS and at least one non-GR-DOS kernel. `make test` runs on DOSBox-X's kernel; `make test-grdos` (M6) boots a GR-DOS image under QEMU and runs `tests/grdos` there.

**Performance:** executable size, resident memory before/after initialization, peak script memory, startup time, representative script runtime, and ability to launch a child program. Clearly distinguish measured values from estimates.

Include automated regression tests wherever practical, with manual/emulated DOS test procedures for behavior not testable on a development host.

### 10.1 Known failures are technical debt

`# todo:` test cases and the entries in `tests/suite/*.baseline` keep the build green while recording behaviour that is wrong today. **They are debt to pay off, not accepted behaviour.** Today's Tiny Tcl debt and where it is due:

| Known failure | Cause | Due |
| --- | --- | --- |
| `tests/tcl16/openplus` (todo); `open` stops in `tests/suite/tinytcl.baseline` | libc16's `fopen` has no update modes (`r+`, `w+`, `a+`) | When libc16 has them; M7 at the latest |

M4 paid off `tests/tinytcl/crlf` (it was todo) and `tests/tinytcl/ctrlz` (which recorded a ^Z run as a command): `source` now reads DOS text.

M5 paid off `tests/dos/dospath` (now `tests/tcl16/dospath`, passing), `tests/tinytcl/glob` (which now expects case-insensitive matches) and the suite's stops for want of `exec`: all of Tcl 6.7's suite runs, and `cd`, `glob`, `info` and `source` finish. The baseline's other entries are DOS deviations, each with its reason in `tests/suite/tinytcl.baseline`, or things Tiny Tcl leaves out (`time`, which stops `exec.tst`).

A milestone that adds a todo or a baseline entry must state why and when it is due. A failure that is truly correct for DOS (for example, no Unix permission bits) moves out of the baseline into a documented DOS deviation in `docs/COMPATIBILITY.md`, with a test that checks the DOS behaviour.

## 11. Milestones

| Milestone | Deliverable | Status |
| --- | --- | --- |
| M1 | Tiny Tcl 6.8 source audit, pinned source, build experiment, and test report | **Done** ([M1-REPORT.md](M1-REPORT.md), PR #1) |
| M2 | Tcl 6.7 DOS port source audit, pinned source, build experiment, and test report | **Done** ([M2-REPORT.md](M2-REPORT.md), PR #2) |
| M3 | Documented interpreter selection, architecture and supported-language baseline | **Done** ([ADR-001](ADR-001-tcl-implementation.md); this document) |
| M4 | Interactive and script-mode `TCL.EXE`, CLI and error conventions | **Done** ([COMPATIBILITY.md](COMPATIBILITY.md)) |
| M5 | DOS filesystem, environment, and process integration | **Done** ([COMPATIBILITY.md](COMPATIBILITY.md), sections 5 and 6) |
| M6 | DOS/GR-DOS introspection using #46 when available | **Done** ([COMPATIBILITY.md](COMPATIBILITY.md), section 7) |
| M7 | 8086, memory, compatibility, packaging, examples, and release validation | Next |

All of M4 to M7 build on Tiny Tcl 6.8.1 with Tcl16's patches.

### M4: `TCL.EXE` (done)

- `src/main.c` is the command line of section 6: REPL with continuation (`% ` and `> `), script with arguments, `-e`, `--`, `/?`, exit statuses 0, N, 1, 2 and 255, and diagnostics on standard error after standard output is flushed. `m1probe` is only in the measurement builds.
- Scripts, `source`d files and REPL input are read as DOS text, CR LF and ^Z (`src/dostext.c`, `src/source.c`, patch 0009). Scripts of 32 KB or more are still read command by command. `tests/tinytcl/crlf` passes; `ctrlz` expects the ^Z to end the script.
- Startup: no banner, no `autoinit.tcl`, no `tcl_library`.
- Stack: 48 KB, measured (section 12).
- [COMPATIBILITY.md](COMPATIBILITY.md) documents the language baseline, the real-mode limits, DOS text and the command line.
- libc16 0.07 is pinned and section 4.2 re-measured; patch 0007 gives `lsort`'s comparator libc16's `__cdecl`. `TCL.EXE` still links Open Watcom's runtime.
- `tests/tcl16` covers the command line, the REPL and DOS text. `tests/run-dos` sees standard error through `ERR2OUT.EXE`.

### M5: DOS filesystem, environment and processes (done)

- **The libc16 build.** `TCL.EXE` links libc16 0.15 and libkitchen16 0.02 and no Open Watcom runtime (sections 4.2 and 5), with Tcl16's `scan` (`src/scan.c`), directory adapter (`src/dosdir.c`) and environment (`src/dosenv.c`); patch 0010. `make measure` builds its variants the same way.
- DOS path semantics in `file` (`src/file.c`; patch 0011), clearing `tests/dos/dospath`, and case-insensitive `glob` (`src/glob.c`, replacing `tcldosgl.c`).
- `file delete`, `mkdir` and `rename`, with Tcl 7's syntax, and `file executable` for what DOS can run.
- `exec` (`src/exec.c`), as in section 8.1: programs through `spawnve`, batch files and COMMAND.COM's own commands through `COMSPEC /C`, pipes through temporary files, standard input from NUL. Memory recovery is a documented limit (section 12).
- All of Tcl 6.7's suite runs, with Tcl16's own `CAT`, `WC` and `PWD` (`tests/tools`), and `tests/suite/tinytcl.baseline` holds only justified entries.

### M6: DOS and GR-DOS introspection (done)

- The `dos` command (`src/dosinfo.c`): `version`, `kernel`, `kernelversion`, `memory` and `drives`, as section 8.2 settles them, with `unknown` and an empty version on kernels that are not GR-DOS.
- GR-DOS identification through gr-dos/gr-dos#46's INT 21h AX=3347h, checked as GR-DOS's `GRVER` checks it.
- `tests/tcl16/dosinfo` on DOSBox-X; `tests/tcl16/dosgr` with `FAKEID.COM` (`tests/tools/fakeid.asm`, after gr-dos's), which imitates GR-DOS's answer and six wrong ones; `make test-grdos` (`tests/run-grdos`), on GR-DOS itself under QEMU, against `GRVER`.

### M7: Validation and release

- 8086 and low-memory validation on GR-DOS and at least one other kernel, and on real hardware where available, including `exec` under real COMMAND.COM.
- The libc16-linked release build, or a documented, measured reason for anything still taken from elsewhere.
- Packaging, `examples/`, the license for Tcl16's own files, and the release.

## 12. Non-goals and open decisions

**Non-goals for the initial release:** Tk, protected-mode dependencies, Tcl 7 or Tcl 8 compatibility, dynamic module loading, incorporation into the resident GR-DOS shell, unnecessary broadening of `libc16`, and maintaining the historical Tcl 6.7 DOS port as a product.

**Settled in M3:** the interpreter (Tiny Tcl 6.8.1; ADR-001), the language level (Tcl 6.7 with TclX's extras), and the component boundaries (section 4).

**Settled in M5:**

- **Conventional-memory recovery: a documented limit.** A child gets the memory above `TCL.EXE`'s heap, which never shrinks. Measured on DOSBox-X (589 KB free before Tcl): `TCL.EXE` takes 152 KB at start, so a program run by a script that has just started gets 431 KB (441,264 bytes, `CHILD.EXE -m`); after the heaviest workload measured, 400 KB taken, about 189 KB is left. libc16 already leaves DOS everything above the heap's highest point (`TCL.EXE` takes 152 KB at start, not all memory), so shrinking the memory block before INT 21h/4Bh would gain nothing; only memory freed inside the heap is lost to a child, and getting it back would mean swapping the heap to disk or EMS, which is not worth its code for the programs a script runs. COMPATIBILITY.md, section 6 documents the limit.
- **Safe process execution**, as the table below asked: INT 21h/4Bh (libc16's `spawnve`) for `.COM` and `.EXE` programs, `COMSPEC /C` only for `.BAT` files and COMMAND.COM's own commands, whose status is then unknown and taken as 0; command tails bounded (126 bytes, 122 through COMMAND.COM); the child's real exit status as `CHILDSTATUS`; redirection and pipes through files; NUL as standard input unless redirected.
- **Floating point: libkitchen16's software floating point**, for every machine, 8087 or not. A float `expr` costs about 51 ms more than an integer one at 4.77 MHz (`fexpr1000` 1,467 ticks against `expr1000` 532), slower than Open Watcom's emulator (35 ms) and much slower than an 8087 (8 ms; M1-REPORT.md, 7.5). Integer work got faster in the same build (`expr1000` 606 to 532 ticks, `fib15` 1,384 to 1,274). Tcl 6's `expr` keeps integers as integers, so only scripts that ask for floating point pay.
- **Formatting: libkitchen16** (section 5).

**Settled in M6:** DOS and GR-DOS introspection, one `dos` command with options, using gr-dos/gr-dos#46's identification call (section 8.2).

**Open engineering decisions,** each to be settled by the milestone named, with measurements:

| Decision | Options and constraints | Milestone |
| --- | --- | --- |
| 16-bit stack and object limits | **Stack size settled in M4: 48 KB** (below). Still open: moving large local buffers to the heap, and whether the 32 KB stops can become Tcl errors | M7 (limits) |
| Remaining libc16 gaps | `fopen`'s update modes (section 10.1) | M7 |
| Minimum RAM and supported upper bounds | 152 KB at start (M5), 154 KB with M6's `dos` (1.6 KB); about 400 KB for the heaviest measured workload | M7 |

**Stack size (settled in M4).** `TCL.EXE` links with a 48 KB stack (`make`'s `STACK`). `make measure` on DOSBox-X's emulated 8086:

| Stack | 16 KB | 32 KB | 40 KB | 48 KB | 56 KB |
| --- | --- | --- | --- | --- | --- |
| Conventional memory taken at start | 185.9 KB | 185.9 KB | 185.9 KB | 185.9 KB | 185.9 KB |
| Deepest simple recursion | 23 | 54 | 69 | 85 | 98 |
| Deepest recursion inside `expr` | 6 | 15 | 19 | 23 | 27 |

A larger stack costs no conventional memory at start: the load image grows, but what `TCL.EXE` takes from DOS does not. 48 KB gives half as much depth again as 32 KB (`fib 15` and 20 levels inside `expr` run; `tests/tcl16/stack`). It leaves about 10 KB of DGROUP, beside 5.3 KB of static data, for growth in M5 to M7 and the libc16 build; 56 KB would leave under 3 KB. The stack guard (patch 0004) and its 3 KB margin are unchanged, and deeper recursion is still a Tcl error.

**The stack with libc16 (M5).** The figures above are Open Watcom's runtime, whose heap starts in the stack's unused room. With libc16 the stack is part of what `TCL.EXE` takes, so each KB of stack is a KB less for the heap and for children:

| Stack | 16 KB | 32 KB | 40 KB | 48 KB | 56 KB |
| --- | --- | --- | --- | --- | --- |
| Conventional memory taken at start | 120.3 KB | 136.3 KB | 144.3 KB | 152.3 KB | 160.3 KB |
| Deepest simple recursion | 23 | 54 | 69 | 84 | 98 |
| Deepest recursion inside `expr` | 7 | 15 | 19 | 23 | 27 |

48 KB stays: the depth it gives is worth more to scripts than 16 KB of memory, and libc16's smaller static data (4.6 KB) leaves DGROUP more room than before.

The guiding rule is to preserve a small, portable, useful Tcl executable while keeping the C runtime minimal. **Any work contributed back to `libc16` must be clean-room implemented.**
