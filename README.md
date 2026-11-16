# tcl16

A Tcl interpreter for 16-bit real-mode DOS (8086/8088, no DOS extender),
built with Open Watcom on Linux, for GR-DOS and other DOS kernels. The
plan is in [`docs/DESIGN.md`](docs/DESIGN.md).

**Status: M2 is done.** M1 and M2 evaluated the two candidate
interpreters:

- Tiny Tcl 6.8.1, in [`docs/M1-REPORT.md`](docs/M1-REPORT.md);
- Peter Sprenger's 1993 DOS port of Tcl 6.7, in
  [`docs/M2-REPORT.md`](docs/M2-REPORT.md).

The inputs are in [`docs/PROVENANCE.md`](docs/PROVENANCE.md). Both build
and pass the tests on an emulated 8086, but only as development builds
linked with Open Watcom's C runtime. M3 chooses between them. Neither is
yet `TCL.EXE` as DESIGN.md describes it (that is M4), and there is no
release.

## Building

You need: `make`, `curl`, `unzip`, `patch` and Perl 5.36 or later. The
DOS tests and measurements also need `dosbox-x`, and `make host` needs
gcc with `-m32` (on Debian or Ubuntu, `gcc-multilib`).

    make                # build/dev/TCL.EXE, the M1 development build (Tiny Tcl)
    make test           # tests/lang, tests/dos, tests/tinytcl under DOSBox-X, emulated 8086
    make host           # Tiny Tcl built for Linux, as a 32-bit-int reference
    make test-host      # tests/lang with the host build
    make libc16-try     # build against libc16; report what it lacks
    make measure        # sizes, memory, stack and speed of the M1 variants
    make tcl67          # build/tcl67/dev/TCL.EXE, the M2 development build (Tcl 6.7 DOS port)
    make test-tcl67     # tests/lang, tests/dos, tests/tcl67 with it
    make test-suite     # Tcl 6.7's own test suite with Tiny Tcl
    make test-suite-tcl67   # ... and with the Tcl 6.7 DOS port
    make libc16-try-tcl67 measure-tcl67   # the M2 experiment and measurements
    make check-vendor   # check vendor/tinytcl-6.8.1 against its manifest
    make help           # every target and variable

`make` fetches and checks the pinned libc16 release
(`scripts/fetch-libc16`) and the Open Watcom snapshot it names. Tiny Tcl
is vendored, unmodified, in `vendor/tinytcl-6.8.1`; the build assembles
it with `patches/` in `build/src`. The Tcl 6.7 DOS port is not vendored:
`make tcl67` and the suite targets fetch it and the Tcl 6.7 release from
SourceForge, check their SHA-256 (`scripts/fetch-tcl67`), and apply
`patches/tcl67/` in `build/tcl67/src`.

## Trying it

    C:\>TCL                      interactive, with the "% " prompt
    C:\>TCL SCRIPT.TCL A B       run a script; argv0, argv and argc are set

This is the test driver for both candidates (`src/main.c`), not the M4
command line. The
dialect is Tcl 6.7: for example, `expr` takes one argument, so brace it
(`expr {$x * 2}`). See M1-REPORT.md, section 9.

## License

Tiny Tcl is under the Tcl license: see the notices in its files and
`vendor/LICENSE.tinytcl`. The Tcl 6.7 DOS port is not redistributed
(PROVENANCE.md explains why). The license for tcl16's own files is not yet
decided.
