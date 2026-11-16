# tcl16

A Tcl interpreter for 16-bit real-mode DOS (8086/8088, no DOS extender),
built with Open Watcom on Linux, for GR-DOS and other DOS kernels. The
plan is in [`docs/DESIGN.md`](docs/DESIGN.md).

**Status: M1 is done.** M1 evaluated the first candidate interpreter,
Tiny Tcl 6.8.1. The findings are in [`docs/M1-REPORT.md`](docs/M1-REPORT.md)
and the inputs in [`docs/PROVENANCE.md`](docs/PROVENANCE.md). Tiny Tcl now
builds and passes the test suite on an emulated 8086, but only as a
development build linked with Open Watcom's C runtime. It is not yet
`TCL.EXE` as DESIGN.md describes it (that is M4), and there is no release.

## Building

You need: `make`, `curl`, `unzip`, `patch` and Perl 5.36 or later. The
DOS tests and measurements also need `dosbox-x`, and `make host` needs
gcc with `-m32` (on Debian or Ubuntu, `gcc-multilib`).

    make                # build/dev/TCL.EXE, the M1 development build
    make test           # tests/lang and tests/dos under DOSBox-X, emulated 8086
    make host           # Tiny Tcl built for Linux, as a 32-bit-int reference
    make test-host      # tests/lang with the host build
    make libc16-try     # build against libc16; report what it lacks
    make measure        # sizes, memory, stack and speed of the M1 variants
    make check-vendor   # check vendor/tinytcl-6.8.1 against its manifest
    make help           # every target and variable

`make` fetches and checks the pinned libc16 release
(`scripts/fetch-libc16`) and the Open Watcom snapshot it names. Tiny Tcl
is vendored, unmodified, in `vendor/tinytcl-6.8.1`; the build assembles
it with `patches/` in `build/src`.

## Trying it

    C:\>TCL                      interactive, with the "% " prompt
    C:\>TCL SCRIPT.TCL A B       run a script; argv0, argv and argc are set

This is the M1 test driver (`src/main.c`), not the M4 command line. The
dialect is Tcl 6.7: for example, `expr` takes one argument, so brace it
(`expr {$x * 2}`). See M1-REPORT.md, section 9.

## License

Tiny Tcl is under the Tcl license: see the notices in its files and
`vendor/LICENSE.tinytcl`. The license for tcl16's own files is not yet
decided.
