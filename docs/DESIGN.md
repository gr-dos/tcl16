# Tcl16 Design

**Project:** `gr-dos/tcl16`  
**Status:** M1 and M2 evaluated; M3 interpreter selection complete ([ADR-001](ADR-001-tcl-implementation.md)); M4 next  
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
- **C runtime:** `libc16` (section 4). The M1 and M2 development builds link Open Watcom's runtime, because libc16 0.04 lacks about twenty functions Tiny Tcl needs (M1-REPORT.md, section 6). Closing that gap is planned work (sections 11 and 12), not a reason to keep Open Watcom's runtime.
- **Memory model:** Large. M1 found no object needs the huge model, because sizes stop at 32 KB. Change only for measured technical reasons.
- **Optional libraries:** Do not require `libkitchen16` or a full C `printf` merely to provide basic Tcl functionality.
- **Build reproducibility:** Pin tool versions and dependencies where practical; provide clear build instructions, deterministic artifact conventions, and `make help` consistent with other GR-DOS projects.

### 3.1 Memory and failure behavior

An 8086's segmented addressing introduces near/far-pointer distinctions and 64 KiB limits on individual segments and objects. Large-model compilation does **not** eliminate these limits or repair unsafe assumptions about pointer arithmetic.

M1 audited Tiny Tcl for these issues and fixed what it found (patches 0002 to 0006; M1-REPORT.md, sections 5 and 7). The limits that remain:

- A string value, a list's text or the interpreter result holds at most 32,767 bytes; one list element about 16 KB; all the words of one command together 32,767 bytes.
- With a 32 KB stack, recursion reaches about 53 levels of a simple proc, and about 15 levels when each level recurses inside `expr`.
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

Linking Tiny Tcl with libc16 0.04 alone leaves these undefined (M1-REPORT.md, section 6):

| Need | Functions | Likely home (to be decided, section 12) |
| --- | --- | --- |
| Formatted output | `sprintf`, `fprintf`, `printf`, `vfprintf` | A compact Tcl16-local formatter (section 5) |
| Conversion and formatted input | `strtod`, `strtol`, `sscanf` | `strtol` fits libc16; `strtod` and `sscanf` depend on the floating-point decision |
| Floating point | 8087 fixups, `__CHP` | See section 12 |
| Directories | `opendir`, `readdir`, `closedir` | A Tcl16-local adapter over libc16's `_dos_findfirst`/`_dos_findnext` |
| Files and directories | `fstat`, `fseek`, `ftell`, `getcwd`, `chdir`, `clearerr` | libc16, where generally useful (`fseek`/`ftell` exist in its 0.05 source) |
| Other | `qsort`, `strcspn`, `isascii`, `abort`, `environ` | libc16, where generally useful |

## 5. Formatting policy

Perl16 may require a relatively comprehensive C-style formatting implementation, potentially placed in the optional `libkitchen16` library. Tcl16 should **not automatically inherit that dependency**.

Tiny Tcl has Tcl's `format` command, implemented by passing each field to C's `sprintf`. Its other uses of `sprintf` are a short list of fixed formats (`%d`, `%ld`, `%u`, `%s`, `%g`, `%.150s`). Tcl's script-level `format` is distinct from C's variadic `printf`. The preferred direction is a compact Tcl16-local formatter covering Tiny Tcl's internal formats and the `format` command's conversions. A separately linkable shared formatting engine is acceptable only if measured code size and behaviour show it is better.

Do not make `libkitchen16` mandatory without an explicit benefit demonstrated by testing; do not assume the linker removes unused functionality more granularly than the object-file layout and linker actually permit.

## 6. User-facing executable

**Name:** `TCL.EXE` (M4). The current `src/main.c` is a test driver, not this command line.

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
- Script execution from a pathname; pass through script arguments in Tcl 6.7's form (`argv0`, `argv`, `argc`, as the test driver already does).
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
| `source` | Required | Present, tested; scripts of any size; CRLF/^Z handling is M4's |
| `catch`, `error` | High | Present, tested |
| Arrays | High | Present, tested |
| `eval`, `uplevel`, `upvar` | High | Present, tested |
| `format` | Desirable; assess footprint | Present, tested; depends on C `sprintf` (section 5) |
| Floating-point expression operations | Evaluate separately | Present with an 8087 or Open Watcom's emulator; future undecided (section 12) |
| `exec` and child programs | Required by section 8 | **Missing**; M5 |
| `file delete`/`mkdir`/`rename` | Desirable | **Missing**; M5 |
| `time`, `history`, `pid`, `switch`, namespaces | Not planned for the baseline | Absent from Tiny Tcl; may be added only with a documented reason |
| Tk or GUI support | Out of scope | — |
| Dynamic binary extension loading | Out of scope initially | — |

Avoid advertising complete Tcl 6.7 compatibility until each command is verified, and never imply modern Tcl compatibility. Tcl 6.x predates namespaces; names containing `::` do **not** imply namespace support. Scripts written for later Tcl versions meet the differences listed in M1-REPORT.md, section 9. M4 starts `docs/COMPATIBILITY.md` from it.

Example target script, contingent on case-insensitive `glob` (M5):

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

Possible commands (names provisional):

```tcl
dos::version
dos::kernel
dos::kernelversion
dos::memory
dos::drives
```

In Tcl 6.7, these would be registered as literal command names, **not** namespace-qualified commands. Choose the final naming convention in M6 after assessing compatibility and usability; simple `dos_version`-style names are an alternative. Avoid adding DOS interrupt wrappers indiscriminately: prefer small, well-specified operations.

Future feature detection should prefer explicit API capabilities over assumptions based solely on a GR-DOS release number.

### 8.3 Deferred capabilities

Serial ports, networking, substantial command-output capture, and dynamic extension systems are not prerequisites for the initial release. Evaluate only after the core interpreter and filesystem/process model work reliably.

## 9. Source layout and dependencies

Current layout:

```text
tcl16/
├── README.md
├── Makefile                 make help lists every target
├── src/                     Tcl16's own C: main.c (test driver, then M4's CLI),
│                            stack.c (stack guard), probe.c (measurement only);
│                            the DOS layer (M4–M6) is added here
├── vendor/
│   ├── tinytcl-6.8.1/       Tiny Tcl, unmodified, with its manifest
│   └── LICENSE.tinytcl
├── patches/                 0001–0006: Tcl16's patches to Tiny Tcl
│   └── tcl67/               M2 reference: patches to the Tcl 6.7 DOS port
├── scripts/                 fetching, building, measuring, test runners
├── tests/
│   ├── lang/ dos/           portable language and DOS cases
│   ├── tinytcl/             Tiny Tcl-specific cases
│   ├── tcl67/               M2 reference cases
│   ├── suite/               baselines for Tcl 6.7's own test suite
│   └── probe/               measurement scripts
└── docs/
    ├── DESIGN.md
    ├── ADR-001-tcl-implementation.md
    ├── M1-REPORT.md  M2-REPORT.md
    └── PROVENANCE.md
```

Planned additions: `docs/COMPATIBILITY.md` (M4, kept current through M7), `docs/PORTING.md` if porting notes outgrow the reports, and `examples/` (M7).

Keep upstream code and Tcl16's own code clearly separated. Respect each source's actual license (PROVENANCE.md). The license for Tcl16's own files is not yet decided and must be settled before a release (M7). The maintained product is a single self-contained DOS executable with no external dynamic runtime. Statically registered Tcl commands are the extension mechanism.

## 10. Validation

Test the Tiny Tcl-based `TCL.EXE` with the shared suites, recording precise build parameters and host/emulator/hardware configurations. The M2 port's suites remain runnable as reference (section 2.2).

**Language:** quoting/braces, substitution, nested evaluation, arithmetic, procedures, scope, loops, lists, error propagation, sourcing, arrays and formatting. Tcl 6.7's own test suite (`make test-suite`) runs against a baseline of expected failures.

**DOS behavior:** paths with drive letters and backslashes, file enumeration, open/read/write/close, CRLF and ^Z scripts, environment variables, command-line arguments, redirection, shell builtins vs executables, exit codes, and low-memory execution.

**Memory and portability:** very small free conventional memory, repeated allocations, nested procedures, 64 KiB boundaries, stack exhaustion, clean shutdown, 8086 compatibility, GR-DOS and at least one non-GR-DOS kernel.

**Performance:** executable size, resident memory before/after initialization, peak script memory, startup time, representative script runtime, and ability to launch a child program. Clearly distinguish measured values from estimates.

Include automated regression tests wherever practical, with manual/emulated DOS test procedures for behavior not testable on a development host.

### 10.1 Known failures are technical debt

`# todo:` test cases and the entries in `tests/suite/*.baseline` keep the build green while recording behaviour that is wrong today. **They are debt to pay off, not accepted behaviour.** Today's Tiny Tcl debt and where it is due:

| Known failure | Cause | Due |
| --- | --- | --- |
| `tests/tinytcl/crlf` (todo) | `source` reads in binary mode, so CRLF breaks continuations | M4 |
| `tests/tinytcl/ctrlz` (records current behaviour) | A trailing ^Z runs as a command | M4 (expected output changes) |
| `tests/dos/dospath` (todo) | `file dirname`/`tail`/`rootname` split only at `/` | M5 |
| `tests/tinytcl/glob` (records current behaviour) | `glob` matches case-sensitively against upper-case names | M5 (expected output changes) |
| `tests/suite/tinytcl.baseline`: `cd`, `exec`, `file`, `glob`, `info`, `open`, `source` stop | No `exec` to build their fixtures | M5, then fix or justify each failure that remains |

A milestone that adds a todo or a baseline entry must state why and when it is due. A failure that is truly correct for DOS (for example, no Unix permission bits) moves out of the baseline into a documented DOS deviation in `docs/COMPATIBILITY.md`, with a test that checks the DOS behaviour.

## 11. Milestones

| Milestone | Deliverable | Status |
| --- | --- | --- |
| M1 | Tiny Tcl 6.8 source audit, pinned source, build experiment, and test report | **Done** ([M1-REPORT.md](M1-REPORT.md), PR #1) |
| M2 | Tcl 6.7 DOS port source audit, pinned source, build experiment, and test report | **Done** ([M2-REPORT.md](M2-REPORT.md), PR #2) |
| M3 | Documented interpreter selection, architecture and supported-language baseline | **Done** ([ADR-001](ADR-001-tcl-implementation.md); this document) |
| M4 | Interactive and script-mode `TCL.EXE`, CLI and error conventions | Next |
| M5 | DOS filesystem, environment, and process integration | Planned |
| M6 | DOS/GR-DOS introspection using #46 when available | Planned |
| M7 | 8086, memory, compatibility, packaging, examples, and release validation | Planned |

All of M4 to M7 build on Tiny Tcl 6.8.1 with Tcl16's patches.

### M4: `TCL.EXE`

- Replace the test driver in `src/main.c` with the command line of section 6: REPL with continuation, script with arguments, `-e`, `/?`, exit statuses and DOS-friendly error output. Leave the measurement command (`m1probe`) out of the release build.
- Read CRLF and ^Z scripts correctly, for the main script and `source` (clears `tests/tinytcl/crlf`, updates `ctrlz`).
- Decide startup behaviour: banner, `tcl_library`, and whether anything like upstream's `autoinit.tcl` is sourced.
- Choose the default stack size, with measured recursion depths (section 12).
- Start `docs/COMPATIBILITY.md` from M1-REPORT.md, section 9.

### M5: DOS filesystem, environment and processes

- DOS path semantics in the `file` subcommands (clears `tests/dos/dospath`), and case-insensitive `glob`.
- `file delete`, `mkdir` and `rename`, as far as DOS permits.
- `exec` over INT 21h/4Bh, as in section 8.1, with memory recovery before a child runs (section 12). Then run the 7 `exec`-dependent files of Tcl 6.7's suite and reduce `tests/suite/tinytcl.baseline` to justified entries.
- Directory enumeration through a Tcl16-local adapter over libc16, as part of the libc16 migration (section 12).

### M6: DOS and GR-DOS introspection

- The commands of section 8.2, their final names, and "unknown / not GR-DOS" on other kernels.
- GR-DOS identification through gr-dos/gr-dos#46 when that API exists. Until then, only the generic DOS facts.

### M7: Validation and release

- 8086 and low-memory validation on GR-DOS and at least one other kernel, and on real hardware where available, including `exec` under real COMMAND.COM.
- The libc16-linked release build, or a documented, measured reason for anything still taken from elsewhere.
- Packaging, `examples/`, the license for Tcl16's own files, and the release.

## 12. Non-goals and open decisions

**Non-goals for the initial release:** Tk, protected-mode dependencies, Tcl 7 or Tcl 8 compatibility, dynamic module loading, incorporation into the resident GR-DOS shell, unnecessary broadening of `libc16`, and maintaining the historical Tcl 6.7 DOS port as a product.

**Settled in M3:** the interpreter (Tiny Tcl 6.8.1; ADR-001), the language level (Tcl 6.7 with TclX's extras), and the component boundaries (section 4).

**Open engineering decisions,** each to be settled by the milestone named, with measurements:

| Decision | Options and constraints | Milestone |
| --- | --- | --- |
| Conventional-memory recovery when spawning a child | The heap never shrinks (M1-REPORT.md, 7.2). Options include shrinking the program's memory block before INT 21h/4Bh, swapping the heap to disk or EMS, or documenting the limit | M5 |
| Safe DOS process execution | Direct INT 21h/4Bh for programs, `COMMAND.COM /C` only for builtins; bounded command tails (126 bytes); real exit status; redirection through files; no console input unless asked | M5 |
| 16-bit stack and object limits | Stack size up to the DGROUP limit (about 58 KB), moving large local buffers to the heap, and whether the 32 KB stops can become Tcl errors | M4 (stack size), M7 (limits) |
| Floating point | Open Watcom's 8087 emulator (not libc16 material), a clean-room software implementation, or integer-only `expr`; an emulated float `expr` costs about 35 ms more than an integer one on an 8088 (M1-REPORT.md, 7.5) | M4 to M7, before the libc16 build |
| Minimal formatting | A compact Tcl16-local formatter for internal formats and the `format` command (section 5), versus a shared engine; never a mandatory `libkitchen16` | Before the libc16 build |
| Remaining libc16 gaps | Section 4.2: which functions belong in libc16 (clean-room) and which in Tcl16-local adapters | M5 to M7 |
| DOS/GR-DOS introspection | Command names and behaviour; depends on gr-dos/gr-dos#46 | M6 |
| Minimum RAM and supported upper bounds | 184 KB at start today; about 430 KB for the heaviest measured workload | M7 |

The guiding rule is to preserve a small, portable, useful Tcl executable while keeping the C runtime minimal. **Any work contributed back to `libc16` must be clean-room implemented.**
