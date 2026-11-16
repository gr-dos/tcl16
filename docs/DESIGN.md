# Tcl16 Design

**Project:** `gr-dos/tcl16` (proposed)  
**Status:** Initial design / interpreter evaluation  
**Target:** Real-mode, 16-bit DOS, including GR-DOS and compatible DOS implementations

## 1. Purpose

Tcl16 is a native 16-bit DOS Tcl interpreter intended to make Tcl scripting useful on vintage PCs, including 8086-class machines. It will be a standalone `TCL.EXE`, not a required component of the GR-DOS kernel or command processor.

The project has two independently evaluated interpreter candidates and a common user-facing design. The eventual choice of interpreter must not determine unrelated command-line, DOS-integration, packaging, or testing decisions.

The primary goals are a useful, reasonably Tcl-compatible scripting environment, a manageable conventional-memory footprint, and compatibility with GR-DOS **and other DOS kernels**.

## 2. Implementation candidates

Evaluate both of the following source bases before selecting one:

| Candidate | Expected advantage | Principal risks |
| --- | --- | --- |
| **Tiny Tcl 6.8** | Small implementation intended for constrained systems | Missing commands or semantic differences from standard Tcl; DOS adaptation |
| **Historical Tcl 6.7 DOS port** | Broader historical Tcl compatibility and existing DOS-oriented code | Conventional-memory use, 16-bit pointer assumptions, runtime dependencies |

These are **candidates, not commitments**. Verify provenance, licensing, source availability, exact feature sets, and the actual viability of building with the project's toolchain. Existing DOS support does not by itself prove compatibility with `libc16` or an 8086.

### 2.1 Evaluation process

1. Acquire and pin exact upstream source versions with their license notices. Record the source location, checksum or commit, patches, and reproduction instructions.
2. Audit C runtime dependencies, code/data memory-model assumptions, allocation, platform-specific sources, command set, and licensing obligations.
3. Try independent builds with Open Watcom targeting real-mode DOS and `libc16` (large model first, unless evidence favors otherwise).
4. Run the same portable test scripts on each candidate.
5. Measure `.EXE` size, startup memory, peak memory for representative workloads, maximum practical script size, and runtime performance on representative CPU configurations.
6. Document deviations from Tcl semantics, missing commands, portability limitations, maintenance cost, and recommended candidate in `docs/COMPATIBILITY.md` or an evaluation report.
7. Choose **one** implementation for the maintained executable; do not carry two indefinitely unless there is a compelling, documented reason.

A successful compile is not sufficient: the implementation must operate reliably with the 8086 segmented memory model.

## 3. Platform and build requirements

- **CPU:** 8086/8088 minimum; no required 286/386 instructions.
- **Execution:** 16-bit DOS real mode. No required DOS extender, DPMI host, or protected-mode runtime.
- **DOS:** Aim for DOS 2.0 and later, subject to verifying actual APIs and utility dependencies. Run on GR-DOS and other compatible kernels.
- **Compiler:** Open Watcom C, following the established GR-DOS toolchain practices.
- **C runtime:** `libc16`.
- **Memory model:** Investigate large model first; change only for measured technical reasons.
- **Optional libraries:** Do not require `libkitchen16` or a full C `printf` merely to provide basic Tcl functionality.
- **Build reproducibility:** Pin tool versions and dependencies where practical; provide clear build instructions, deterministic artifact conventions, and `make help` consistent with other GR-DOS projects.

### 3.1 Memory and failure behavior

An 8086's segmented addressing introduces near/far-pointer distinctions and 64 KiB limits on individual segments and objects. Large-model compilation does **not** eliminate these limits or repair unsafe assumptions about pointer arithmetic.

Audit interpreter allocation and string handling for:

- Near/far pointer validity, comparisons, and casts.
- Object lengths and offsets that could exceed 16-bit ranges.
- Stack use, interpreter nesting, and recursion.
- Small-allocation fragmentation and heap exhaustion.
- Memory retained while invoking child DOS programs.

Allocation failures must report a useful error where recovery is possible; they must not silently corrupt interpreter state. State any documented limits rather than suggesting support for arbitrarily large objects.

## 4. `libc16` integration and clean-room policy

`libc16` is the minimal common C runtime. Tcl16 should use its existing interfaces wherever possible, and **must not expand `libc16` solely for convenience** if a small Tcl16-local adapter is more appropriate.

### Mandatory clean-room requirement

**Any additions or changes to `libc16` made to support Tcl16 must be clean-room implemented.** Implement them independently from documented behavior, public interface specifications, and original test cases; **do not copy, translate, or mechanically adapt source from the historical Tcl implementations, DOS runtime libraries, other libc implementations, or other third-party code into `libc16`.** When reviewing upstream source to identify required behavior, maintain a clear separation between requirements and independent implementation. Record the specification basis, authorship, and tests for any new `libc16` facility.

Any legitimately reused or vendored interpreter source belongs in Tcl16 under its own applicable license, not silently incorporated into `libc16` as though it were original code. A dependency's license being permissive does **not** waive the project's clean-room requirement for `libc16`.

Additional rules:

- Add a facility to `libc16` only if it is generally useful to 16-bit C applications and fits that library's minimal-runtime scope.
- Prefer separate reusable components, including `libkitchen16` for heavier optional functionality, over increasing every application's baseline footprint.
- Verify the existing `libc16` API before proposing an addition (e.g., allocation, stdio, environment, execution, directory enumeration).
- Supply focused tests under GR-DOS and at least one other compatible DOS environment for each changed runtime facility.
- Keep Tcl-specific command semantics inside Tcl16, not inside the C library.

## 5. Formatting policy

Perl16 may require a relatively comprehensive C-style formatting implementation, potentially placed in the optional `libkitchen16` library. Tcl16 should **not automatically inherit that dependency**.

Tcl's script-level `format` command is distinct from C's variadic `printf`/`sprintf`. First determine whether each interpreter provides `format` and what Tcl-compatible semantics it supports. If implementation is required, prefer a compact Tcl-specific solution, unless measured code size and behavior show that a separately linkable shared formatting engine is advantageous.

Do not make `libkitchen16` mandatory without an explicit benefit demonstrated by testing; do not assume the linker removes unused functionality more granularly than the object-file layout and linker actually permit.

## 6. User-facing executable

**Name:** `TCL.EXE`

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
- Script execution from a pathname; pass through script arguments in a documented form appropriate to the supported Tcl version.
- Command-line expression/script execution via `-e` as a Tcl16-specific convenience.
- Concise `/?` usage output, DOS-friendly errors, and meaningful exit statuses.

Preserve standard Tcl parsing and evaluation semantics whenever supported. Clearly document any CLI options or behaviors introduced by Tcl16 rather than implying they are historical Tcl features. DOS path syntax, quoting, command-tail limits, and standard streams require explicit tests.

## 7. Minimum Tcl language capabilities

The intended baseline is a coherent, practically useful Tcl subset, not a new Tcl-inspired language:

| Capability | Importance |
| --- | --- |
| Variables and substitutions; `set`, `unset` | Required |
| `if`, `while`, `for`, `foreach` | Required |
| `proc`, `return`, procedure scopes | Required |
| `expr` and common integer operators | Required |
| Lists and common list operations | Required |
| Strings and common string operations | Required |
| `source` | Required |
| `catch`, `error` | High |
| Arrays | High |
| `eval`, `uplevel`, `upvar` | High |
| `format` | Desirable; assess footprint |
| Floating-point expression operations | Evaluate separately; not an unconditional baseline |
| Tk or GUI support | Out of scope |
| Dynamic binary extension loading | Out of scope initially |

The actual supported command set must be established from tests and documented, particularly if Tiny Tcl is selected. Avoid advertising complete Tcl 6.7 or modern Tcl compatibility unless verified. Tcl 6.x predates modern Tcl namespace features; names containing `::` do **not** imply namespace support.

Example target script, contingent on the specified file commands being implemented:

```tcl
set count 0
foreach file [glob *.*] {
    puts $file
    incr count
}
puts "Total files: $count"
```

## 8. DOS integration layer

Keep DOS-specific services separate from the interpreter core so the Tcl engine remains understandable and the DOS interfaces can be tested independently.

### 8.1 Initial facilities

- Filesystem operations, directory enumeration/globbing, DOS path handling, and file I/O.
- Access to process environment variables and working directory.
- Execution of external DOS programs, with clear documentation of conventional-memory constraints.
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

In Tcl 6.x, these may be registered as literal command names, **not** namespace-qualified commands. Choose the final naming convention after assessing compatibility and usability; simple `dos_version`-style names are an alternative. Avoid adding DOS interrupt wrappers indiscriminately: prefer small, well-specified operations.

Future feature detection should prefer explicit API capabilities over assumptions based solely on a GR-DOS release number.

### 8.3 Deferred capabilities

Serial ports, networking, substantial command-output capture, and dynamic extension systems are not prerequisites for the initial release. Evaluate only after the core interpreter and filesystem/process model work reliably.

## 9. Source layout and dependencies

Proposed layout (adjust after choosing a source base):

```text
tcl16/
├── README.md
├── Makefile
├── src/
│   ├── main.c
│   ├── dos.c
│   └── grdos.c
├── vendor/              # Pinned upstream interpreter, notices, patches
├── tests/
├── examples/
└── docs/
    ├── DESIGN.md
    ├── COMPATIBILITY.md
    └── PORTING.md
```

Keep upstream code and project-specific adapters clearly separated whenever practical. Respect each source's actual license; independently verify that the chosen license obligations are compatible with project distribution.

The maintained product is a single self-contained DOS executable with no external dynamic runtime. Statically registered Tcl commands are the initial extension mechanism.

## 10. Validation

Test both interpreter candidates against a common suite, recording precise build parameters and host/emulator/hardware configurations.

**Language:** quoting/braces, substitution, nested evaluation, arithmetic, procedures, scope, loops, lists, error propagation, sourcing, and optional arrays/formatting.

**DOS behavior:** paths with drive letters and backslashes, file enumeration, open/read/write/close, environment variables, command-line arguments, redirection, shell builtins vs executables, exit codes, and low-memory execution.

**Memory and portability:** very small free conventional memory, repeated allocations, nested procedures, 64 KiB boundaries, stack exhaustion, clean shutdown, 8086 compatibility, GR-DOS and at least one non-GR-DOS kernel.

**Performance:** executable size, resident memory before/after initialization, peak script memory, startup time, representative script runtime, and ability to launch a child program. Clearly distinguish measured values from estimates.

Include automated regression tests wherever practical, with manual/emulated DOS test procedures for behavior not testable on a development host.

## 11. Milestones

| Milestone | Deliverable |
| --- | --- |
| M1 | Tiny Tcl 6.8 source audit, pinned source, build experiment, and test report |
| M2 | Tcl 6.7 DOS port source audit, pinned source, build experiment, and test report |
| M3 | Documented candidate selection and supported-language baseline |
| M4 | Interactive and script-mode `TCL.EXE`, CLI and error conventions |
| M5 | DOS filesystem, environment, and process integration |
| M6 | DOS/GR-DOS introspection using #46 when available |
| M7 | 8086, memory, compatibility, packaging, examples, and release validation |

Milestones may overlap, but the user-facing design should remain consistent across candidate experiments.

## 12. Non-goals and open decisions

**Non-goals for the initial release:** Tk, protected-mode dependencies, complete modern Tcl compatibility, dynamic module loading, incorporation into the resident GR-DOS shell, and unnecessary broadening of `libc16`.

**Open decisions:**

- Which interpreter gives the best tested feature/footprint tradeoff?
- How should legacy Tcl command coverage be prioritized if neither candidate meets the baseline without additions?
- Which Tcl16 DOS command names should be exposed?
- How should stdout capture and child-process memory recovery be implemented, if feasible?
- Is a compact standalone Tcl formatter better than a selectively linked `libkitchen16` component?
- What are the minimum practical RAM requirements and supported upper bounds on script/object sizes?

The guiding rule is to preserve a small, portable, useful Tcl executable while keeping the C runtime minimal. **Any work contributed back to `libc16` must be clean-room implemented.**
