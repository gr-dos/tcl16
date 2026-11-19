# tcl16

A Tcl 6.7-compatible interpreter for 16-bit real-mode DOS (8086/8088,
no DOS extender), based on [Tiny Tcl 6.8.1](docs/PROVENANCE.md), built
with Open Watcom on Linux, for GR-DOS and other DOS kernels. It is not
compatible with Tcl 7 or Tcl 8. The plan is in
[`docs/DESIGN.md`](docs/DESIGN.md).

**Status: release 0.01 (M7).** Download `tcl.exe` from the
[releases](https://github.com/gr-dos/tcl16/releases).

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
- **M5** linked `TCL.EXE` with libc16 and libkitchen16 instead of Open
  Watcom's runtime, and added DOS paths in `file`, case-insensitive
  `glob`, `file delete`/`mkdir`/`rename` and `exec`.
- **M6** added the `dos` command: the DOS version, the kernel (GR-DOS,
  through its identification call, or `unknown`), GR-DOS's release, free
  memory and drives (COMPATIBILITY.md, section 7).
- **M7** validated `TCL.EXE` on MS-DOS 4.00 and GR-DOS as well as
  DOSBox-X's 8086, and with little memory; added `open`'s update modes
  (libc16 0.17), `examples/`, the license and the release.

`TCL.EXE` is Tiny Tcl with Tcl16's patches and DOS layer, linked with
libc16 and libkitchen16 and no other runtime. It passes the language, DOS,
command-line and example tests on DOSBox-X's emulated 8086, on MS-DOS
4.00 and on GR-DOS, and Tcl 6.7's own suite apart from documented DOS
deviations (DESIGN.md, sections 10 and 11). It needs about 175 KB of free
conventional memory and `FILES=20` (COMPATIBILITY.md, section 8). It has
not yet been run on real hardware.
The inputs are in [`docs/PROVENANCE.md`](docs/PROVENANCE.md).

The Tcl 6.7 DOS port is kept only as reference material from M2. Its
`make` targets still work, but it is not part of the product.

## Building

You need: `make`, `curl`, `unzip`, `patch` and Perl 5.36 or later. The
DOS tests and measurements also need `dosbox-x`, and `make host` needs
gcc with `-m32` (on Debian or Ubuntu, `gcc-multilib`).

    make                # build/dev/TCL.EXE, the development build
    make test           # tests/lang, dos, tinytcl, tcl16 under DOSBox-X, emulated 8086
    make test-grdos     # make test's cases on GR-DOS, booted under QEMU
    make test-msdos     # ... and on MS-DOS 4.00
    make host           # Tiny Tcl built for Linux, as a 32-bit-int reference
    make test-host      # tests/lang with the host build
    make libc16-try     # build against libc16; report what it lacks
    make measure        # sizes, memory, stack and speed of the M1 variants
    make test-suite     # Tcl 6.7's own test suite with TCL.EXE
    make check-vendor   # check vendor/tinytcl-6.8.1 against its manifest
    make help           # every target and variable

The M2 reference build of the Tcl 6.7 DOS port:

    make tcl67          # build/tcl67/dev/TCL.EXE
    make test-tcl67     # tests/lang, tests/dos, tests/tcl67 with it
    make test-suite-tcl67   # Tcl 6.7's own test suite with it
    make libc16-try-tcl67 measure-tcl67   # the M2 experiment and measurements

`make test-grdos` and `make test-msdos` also need mtools,
`qemu-system-i386` and bootable floppy images from a gr-dos build: by
default `../gr-dos/build/linux/tests/grver-grdos.img` (GR-DOS, with its
identification call and `GRVER.EXE`) and `grver-msdos.img` (MS-DOS
4.00), which gr-dos's `make test-grver` makes; set `GRDOS_IMAGE` or
`MSDOS_IMAGE` for others.

`make` fetches and checks the pinned libc16 and libkitchen16 releases
(`scripts/fetch-libc16`, `scripts/fetch-libkitchen16`) and the Open
Watcom snapshot libc16 names. Tiny Tcl
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
^Z. A script can ask what it runs on:

    if {[dos kernel] == "GR-DOS"} {
        puts "GR-DOS [dos kernelversion], [dos memory] bytes free"
    }

More in [`examples/`](examples): a directory listing, `wc`, a text
search, system information and running a program.

The dialect is Tcl 6.7: for example, `expr` takes one argument, so
brace it (`expr {$x * 2}`), and there is no `switch` (use `case`).
[`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md) has the details.

## Releasing

The version is the one line in `VERSION`, DOS style (0.01, 0.02, ...
1.00), not semantic versioning; `TCL /?` reports it. When a PR that
changes it is merged to main, the release workflow
(`.github/workflows/release.yml`) builds and tests the merge commit, tags
it `v<VERSION>` and publishes a GitHub release with `tcl.exe`,
`LICENSE.TXT` (the notices `TCL.EXE` needs: Tcl16's, Tiny Tcl's, and the
MIT notices of libc16 and libkitchen16) and `tcl16-<VERSION>.zip`, the
two together; `make dist` makes the last two in `build/dist`.

## License

tcl16's own files are under the Tcl license (`LICENSE`). Tiny Tcl is under
the Tcl license too: see the notices in its files and
`vendor/LICENSE.tinytcl`. `TCL.EXE` also contains libc16 and
libkitchen16, under the MIT License. The Tcl 6.7 DOS port is not
redistributed (PROVENANCE.md explains why).
