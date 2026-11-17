# tcl16

A Tcl 6.7-compatible interpreter for 16-bit real-mode DOS (8086/8088,
no DOS extender), based on [Tiny Tcl 6.8.1](docs/PROVENANCE.md), built
with Open Watcom on Linux, for GR-DOS and other DOS kernels. It is not
compatible with Tcl 7 or Tcl 8. The plan is in
[`docs/DESIGN.md`](docs/DESIGN.md).

**Status: M4 is done; M5 is next.** There is no release yet.

- **M1** evaluated Tiny Tcl 6.8.1:
  [`docs/M1-REPORT.md`](docs/M1-REPORT.md).
- **M2** evaluated Peter Sprenger's 1993 DOS port of Tcl 6.7:
  [`docs/M2-REPORT.md`](docs/M2-REPORT.md).
- **M3** chose Tiny Tcl 6.8.1 as tcl16's foundation:
  [`docs/ADR-001-tcl-implementation.md`](docs/ADR-001-tcl-implementation.md).
  The two share the same Tcl 6.7 core. Tiny Tcl is smaller, adds TclX
  commands, sources scripts of 32 KB or more, and has a clear license.
  The port's DOS layer would have to be rewritten anyway.
- **M4** built `TCL.EXE`'s command line and REPL, and reads scripts as
  DOS text: [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md).

Today's `TCL.EXE` has Tcl16's command line, but it is still a
development build: Tiny Tcl with Tcl16's patches and DOS text handling,
linked with Open Watcom's C runtime rather than libc16. It passes the
language, DOS and command-line tests on an emulated 8086, apart from
known failures that are scheduled to be fixed (DESIGN.md, section
10.1). It has no `exec`, and DOS paths and filename case are not
handled yet. M5 to M7 build the rest of the DOS layer, GR-DOS
introspection and the libc16 release build (DESIGN.md, section 11).
The inputs are in [`docs/PROVENANCE.md`](docs/PROVENANCE.md).

The Tcl 6.7 DOS port is kept only as reference material from M2. Its
`make` targets still work, but it is not part of the product.

## Building

You need: `make`, `curl`, `unzip`, `patch` and Perl 5.36 or later. The
DOS tests and measurements also need `dosbox-x`, and `make host` needs
gcc with `-m32` (on Debian or Ubuntu, `gcc-multilib`).

    make                # build/dev/TCL.EXE, the development build
    make test           # tests/lang, dos, tinytcl, tcl16 under DOSBox-X, emulated 8086
    make host           # Tiny Tcl built for Linux, as a 32-bit-int reference
    make test-host      # tests/lang with the host build
    make libc16-try     # build against libc16; report what it lacks
    make measure        # sizes, memory, stack and speed of the M1 variants
    make test-suite     # Tcl 6.7's own test suite with Tiny Tcl
    make check-vendor   # check vendor/tinytcl-6.8.1 against its manifest
    make help           # every target and variable

The M2 reference build of the Tcl 6.7 DOS port:

    make tcl67          # build/tcl67/dev/TCL.EXE
    make test-tcl67     # tests/lang, tests/dos, tests/tcl67 with it
    make test-suite-tcl67   # Tcl 6.7's own test suite with it
    make libc16-try-tcl67 measure-tcl67   # the M2 experiment and measurements

`make` fetches and checks the pinned libc16 release
(`scripts/fetch-libc16`) and the Open Watcom snapshot it names. Tiny Tcl
is vendored, unmodified, in `vendor/tinytcl-6.8.1`; the build assembles
it with `patches/` in `build/src`. The Tcl 6.7 DOS port is not vendored:
`make tcl67` and the suite targets fetch it and the Tcl 6.7 release from
SourceForge, check their SHA-256 (`scripts/fetch-tcl67`), and apply
`patches/tcl67/` in `build/tcl67/src`. Its archive also carries Tcl 6.7's own
test suite.

## Trying it

    C:\>TCL                      interactive: "% " prompt, "> " to continue
    C:\>TCL SCRIPT.TCL A B       run a script; argv0, argv and argc are set
    C:\>TCL -e "puts [pwd]"      run the commands given (a Tcl16 extension)
    C:\>TCL /?                   usage

    C:\>TCL
    % set x 10
    10
    % expr {$x * 2}
    20
    % proc twice {n} {
    >     return [expr {$n * 2}]
    > }
    % twice 21
    42
    % exit

End the REPL with `exit` or Ctrl-Z. An uncaught error in a script
prints `Error:` and where it happened on standard error, and exits
with status 1. Scripts may have DOS (CR LF) line ends and a trailing
^Z. The dialect is Tcl 6.7: for example, `expr` takes one argument, so
brace it (`expr {$x * 2}`), and there is no `switch` (use `case`).
[`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) has the details.

## License

Tiny Tcl is under the Tcl license: see the notices in its files and
`vendor/LICENSE.tinytcl`. The Tcl 6.7 DOS port is not redistributed
(PROVENANCE.md explains why). The license for tcl16's own files is not yet
decided.
