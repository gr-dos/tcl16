# tcl16: a Tcl interpreter for 16-bit real-mode DOS (docs/DESIGN.md).
# `make help` lists the targets and variables (HELP, below).
#
# M1 evaluates the first candidate, Tiny Tcl 6.8.1 (docs/M1-REPORT.md).
# Its source is vendored, unmodified, in vendor/tinytcl-6.8.1
# (docs/PROVENANCE.md); build/src is that tree with CRLF turned into LF
# and patches/*.patch applied in order. tcl16's own C files are in src/.
#
# M2 evaluates the second, Peter Sprenger's 1993 MS-DOS port of Tcl 6.7
# (docs/M2-REPORT.md). It is fetched, not vendored (scripts/fetch-tcl67;
# docs/PROVENANCE.md), into build/tcl67; build/tcl67/src is its sources
# with patches/tcl67/*.patch applied, and build/tcl67/dev/TCL.EXE its
# development build. The *-tcl67 targets are M2's counterparts of M1's.
#
# build/dev/TCL.EXE is the M1 development build: large model, 8086
# instructions, linked with Open Watcom's C runtime, because libc16 does
# not yet provide what the interpreter needs (make libc16-try; M1-REPORT,
# section 6). It is an evaluation binary, not a release.
#
# The Open Watcom toolchain is the snapshot pinned by libc16, fetched by
# the libc16 tree's tools/fetch-watcom into build/tools, as in the other
# gr-dos *16 projects.
#
# Needs make, curl, unzip, patch, Perl 5.36 or later and, for the DOS
# tests and probes, dosbox-x; make host needs gcc with -m32 support.

VENDOR      := vendor/tinytcl-6.8.1
SRC         := build/src
DEV         := build/dev

LIBC16_ROOT ?= build/libc16
LIBC16_DIR  := $(if $(wildcard $(LIBC16_ROOT)/build/stage/VERSION),$(LIBC16_ROOT)/build/stage,$(LIBC16_ROOT))
LIBC16L     := $(LIBC16_DIR)/lib/LIBC16L.LIB
TOOLS_ENV   := build/tools.env
WATCOM_ENV   = . ./$(TOOLS_ENV) && PATH=$$WATCOM/binl64:$$PATH

# Tiny Tcl's modules, as upstream's Makefile and tlib.rsp list them, less
# tinytcl.c (replaced by src/main.c), borland.c (Borland's _stklen; the
# stack size is a link option here) and dos.c (Borland BIOS and conio
# commands, left to M5 and M6; M1-REPORT, section 3).
TCL_NAMES   = tclGet tclProc tclVar tclAssem tclCmdAH tclCmdMZ tclHash \
              tclParse panic regexp tclCmdIL tclalloc tclBasic tclExpr \
              tclUtil tclEnv tcldosaz tcldosut tcldosst tcldosgl tclXdbg \
              tclXgen readdir
# tcl16's own, in src/.
T16_NAMES   = main stack dostext source probe

# Development build: large model, 8086 instructions, Open Watcom's
# headers and runtime, stack checking on. -zc puts string literals in the
# code segment, out of DGROUP, which in the large model also holds the
# stack (M1-REPORT, section 7).
CC          = wcc
OPT        ?= -os
STACK      ?= 32k
DEV_CFLAGS  = -q -zq -ml -0 -zc $(OPT) -bt=dos -DTCL16_PROBE -DTCL16_STACKAVAIL \
              -I$(SRC) \
              -I$$WATCOM/h
DEV_LDOPTS  = option quiet, stack=$(STACK)

DEV_OBJS    = $(TCL_NAMES:%=$(DEV)/%.obj) $(T16_NAMES:%=$(DEV)/%.obj)
PATCHES     = $(sort $(wildcard patches/*.patch))

all: $(DEV)/TCL.EXE

$(DEV)/TCL.EXE: $(DEV_OBJS) $(TOOLS_ENV)
	$(WATCOM_ENV) && wlink $(DEV_LDOPTS), map=$(DEV)/tcl.map \
	  system dos name $@ $(DEV_OBJS:%=file %)

$(DEV)/%.obj: $(SRC)/%.c $(SRC)/.stamp $(TOOLS_ENV)
	mkdir -p $(DEV)
	$(WATCOM_ENV) && $(CC) $(DEV_CFLAGS) -fo=$@ $<

$(DEV)/%.obj: src/%.c $(SRC)/.stamp $(TOOLS_ENV)
	mkdir -p $(DEV)
	$(WATCOM_ENV) && $(CC) $(DEV_CFLAGS) -fo=$@ $<

$(SRC)/%.c: $(SRC)/.stamp ;

# The assembled source tree: the vendored C sources with LF line ends
# (the patches are LF), then the patches, with no fuzz.
$(SRC)/.stamp: $(VENDOR)/tclBasic.c $(PATCHES)
	rm -rf $(SRC) $(SRC).tmp
	mkdir -p $(SRC).tmp
	cp $(VENDOR)/*.c $(VENDOR)/*.h $(SRC).tmp/
	for f in $(SRC).tmp/*; do tr -d '\r' < $$f > $$f.lf && mv $$f.lf $$f; done
	for p in $(PATCHES); do \
	  patch -s -p1 -F0 -d $(SRC).tmp < $$p || exit 1; done
	mv $(SRC).tmp $(SRC)
	touch $@

# The pinned libc16 release (scripts/fetch-libc16), for the toolchain pin
# and the libc16 experiment. A LIBC16_ROOT given on the command line must
# already be built.
ifeq ($(LIBC16_ROOT),build/libc16)
$(LIBC16L): scripts/fetch-libc16
	sh scripts/fetch-libc16 $(LIBC16_ROOT)
endif

# The Open Watcom snapshot libc16 names, as shell assignments.
$(TOOLS_ENV): $(LIBC16L)
	mkdir -p build
	watcom=$$(sh $(LIBC16_DIR)/tools/fetch-watcom $(CURDIR)/build/tools) \
	  && printf 'WATCOM=%s\nexport WATCOM\n' "$$watcom" > $@

# ERR2OUT.EXE runs a command with its standard error sent to its standard
# output, so that a test can see diagnostics (tests/run-dos). A test tool,
# never published.
$(DEV)/ERR2OUT.EXE: tests/tools/err2out.c $(TOOLS_ENV)
	mkdir -p $(DEV)
	$(WATCOM_ENV) && $(CC) -q -zq -ms -0 -bt=dos -I$$WATCOM/h \
	  -fo=$(DEV)/err2out.obj $< && \
	  wlink option quiet system dos name $@ file $(DEV)/err2out.obj

test: $(DEV)/TCL.EXE $(DEV)/ERR2OUT.EXE
	perl tests/run-dos -e $(DEV)/ERR2OUT.EXE $(DEV)/TCL.EXE tests/lang \
	  tests/dos tests/tinytcl tests/tcl16

# The same language tests, with Tiny Tcl built for the host (32-bit int):
# a reference for what is a 16-bit problem and what is Tiny Tcl's own.
host: build/host/tclhost

build/host/tclhost: $(SRC)/.stamp src/main.c src/stack.c src/dostext.c \
  src/source.c src/tcl16.h scripts/build-host
	sh scripts/build-host $(SRC) build/host

test-host: build/host/tclhost
	perl tests/run-host build/host/tclhost tests/lang

libc16-try: $(SRC)/.stamp $(LIBC16L) $(TOOLS_ENV)
	$(WATCOM_ENV) && sh scripts/try-libc16 $(SRC) $(LIBC16_DIR) \
	  build/libc16-try $(TCL_NAMES:%=$(SRC)/%.c) src/main.c src/stack.c \
	  src/dostext.c src/source.c

# M1 measurements (docs/M1-REPORT.md, section 7): the variants' sizes and
# the probe scripts' memory, stack and time figures under DOSBox-X.
VARIANTS    = os ox od stk16 stk48

variants: $(SRC)/.stamp $(TOOLS_ENV)
	rm -f build/var/FREEMEM.EXE
	$(WATCOM_ENV) && for v in $(VARIANTS); do \
	  sh scripts/build-variant $$v || exit 1; done

measure: variants
	perl scripts/measure build/var $(VARIANTS) os+nofpu

# M2: the Tcl 6.7 DOS port. Its modules, as the port's TCL.PRJ lists
# them, under Tcl 6.7's file names (scripts/fetch-tcl67); tclTest.c, the
# upstream test driver, is replaced by src/main.c.
T67         := build/tcl67
T67_SRC     := $(T67)/src
T67_DEV     := $(T67)/dev
T67_NAMES   = msdos tclUnixAZ tclUnixStr tclUnixUtil tclGlob tclEnv panic \
              tclAssem tclBasic tclCkalloc tclCmdAH tclCmdIL tclCmdMZ \
              tclExpr tclGet tclHash tclHistory tclParse tclProc regexp \
              tclUtil tclVar
T67_CFLAGS  = $(subst -I$(SRC),-I$(T67_SRC),$(DEV_CFLAGS)) -DTCL16_TCL67
# Its driver is the M1/M2 test driver, frozen as src/tcl67main.c; Tcl16's
# DOS layer (src/dostext.c, src/source.c) and command line are not built.
T67_T16     = tcl67main stack probe
T67_OBJS    = $(T67_NAMES:%=$(T67_DEV)/%.obj) $(T67_T16:%=$(T67_DEV)/%.obj)
T67_PATCHES = $(sort $(wildcard patches/tcl67/*.patch))

tcl67: $(T67_DEV)/TCL.EXE

$(T67)/.fetched: scripts/fetch-tcl67
	sh scripts/fetch-tcl67 $(T67)

$(T67_DEV)/TCL.EXE: $(T67_OBJS) $(TOOLS_ENV)
	$(WATCOM_ENV) && wlink $(DEV_LDOPTS), map=$(T67_DEV)/tcl.map \
	  system dos name $@ $(T67_OBJS:%=file %)

$(T67_DEV)/%.obj: $(T67_SRC)/%.c $(T67_SRC)/.stamp $(TOOLS_ENV)
	mkdir -p $(T67_DEV)
	$(WATCOM_ENV) && $(CC) $(T67_CFLAGS) -fo=$@ $<

$(T67_DEV)/%.obj: src/%.c $(T67_SRC)/.stamp $(TOOLS_ENV)
	mkdir -p $(T67_DEV)
	$(WATCOM_ENV) && $(CC) $(T67_CFLAGS) -fo=$@ $<

$(T67_SRC)/%.c: $(T67_SRC)/.stamp ;

$(T67_SRC)/.stamp: $(T67)/.fetched $(T67_PATCHES)
	rm -rf $(T67_SRC) $(T67_SRC).tmp
	cp -R $(T67)/upstream $(T67_SRC).tmp
	for p in $(T67_PATCHES); do \
	  patch -s -p1 -F0 -d $(T67_SRC).tmp < $$p || exit 1; done
	mv $(T67_SRC).tmp $(T67_SRC)
	touch $@

test-tcl67: $(T67_DEV)/TCL.EXE
	WORK=$(T67)/run-dos perl tests/run-dos $(T67_DEV)/TCL.EXE \
	  tests/lang tests/dos tests/tcl67

# Tcl 6.7's own test suite, from the port's archive (scripts/run-tcl-suite),
# with each candidate: the language both share, measured the same way.
test-suite-tcl67: $(T67_DEV)/TCL.EXE
	perl scripts/run-tcl-suite $(T67_DEV)/TCL.EXE $(T67)/port/TESTS \
	  $(T67)/port/UTIL $(T67)/suite tests/suite/tcl67.baseline

test-suite: $(DEV)/TCL.EXE $(T67)/.fetched
	perl scripts/run-tcl-suite $(DEV)/TCL.EXE $(T67)/port/TESTS \
	  $(T67)/port/UTIL build/suite tests/suite/tinytcl.baseline

libc16-try-tcl67: $(T67_SRC)/.stamp $(LIBC16L) $(TOOLS_ENV)
	$(WATCOM_ENV) && TRY_DEFS=-DTCL16_TCL67 sh scripts/try-libc16 \
	  $(T67_SRC) $(LIBC16_DIR) $(T67)/libc16-try \
	  $(T67_NAMES:%=$(T67_SRC)/%.c) src/tcl67main.c src/stack.c

variants-tcl67: $(T67_SRC)/.stamp $(TOOLS_ENV)
	rm -f $(T67)/var/FREEMEM.EXE
	$(WATCOM_ENV) && for v in $(VARIANTS); do \
	  VAR_SRC=$(T67_SRC) VAR_OUT=$(T67)/var VAR_NAMES="$(T67_NAMES)" \
	  VAR_DEFS=-DTCL16_TCL67 VAR_T16="$(T67_T16)" \
	  sh scripts/build-variant $$v || exit 1; done

measure-tcl67: variants-tcl67
	MEASURE_OUT=$(T67)/measure perl scripts/measure $(T67)/var \
	  $(VARIANTS) os+nofpu

check-vendor:
	sh scripts/check-vendor

check-vendor-zip:
	sh scripts/check-vendor --zip

clean:
	rm -rf $(SRC) $(SRC).tmp $(DEV) build/var build/host build/libc16-try \
	  build/run-dos build/run-host build/measure
	rm -rf $(T67_SRC) $(T67_SRC).tmp $(T67_DEV) $(T67)/var \
	  $(T67)/libc16-try $(T67)/run-dos $(T67)/measure $(T67)/suite \
	  build/suite

help:
	$(info $(HELP))
	@:

define HELP
tcl16 build (M1: Tiny Tcl 6.8.1; M2: Tcl 6.7 DOS port).
Usage: make [TARGET] [VARIABLE=value ...]

Build
  make, make all      build/dev/TCL.EXE, the M1 development build (large
                      model, 8086, Open Watcom's C runtime; not a release)
  make host           build/host/tclhost, Tiny Tcl for Linux (gcc -m32), as
                      a 32-bit-int reference for the tests
  make clean          remove build products; keep downloads and tools
  make help           this list

Tests
  make test           tests/lang, tests/dos and tests/tinytcl with
                      build/dev/TCL.EXE under DOSBox-X, on an emulated
                      8086 (tests/run-dos)
  make test-host      tests/lang with build/host/tclhost (tests/run-host)
  make test-suite     Tcl 6.7's own test suite (from the M2 download) with
                      build/dev/TCL.EXE, against tests/suite/tinytcl.baseline

Evaluation (docs/M1-REPORT.md)
  make libc16-try     compile against libc16's headers and link with
                      LIBC16L.LIB; report what is missing
                      (build/libc16-try/report.txt)
  make variants       the measurement builds in build/var
  make measure        sizes, memory, stack depth and timings of the variants
  make check-vendor   check vendor/tinytcl-6.8.1 against its manifest
  make check-vendor-zip   and against the upstream release zip (network)

M2: the Tcl 6.7 DOS port (docs/M2-REPORT.md), fetched into build/tcl67
  make tcl67          build/tcl67/dev/TCL.EXE, its development build
  make test-tcl67     tests/lang, tests/dos and tests/tcl67 with it
  make test-suite-tcl67   Tcl 6.7's own test suite with it, against
                      tests/suite/tcl67.baseline
  make libc16-try-tcl67   the libc16 experiment for it
                      (build/tcl67/libc16-try/report.txt)
  make variants-tcl67, make measure-tcl67
                      its measurement builds and figures (build/tcl67/var,
                      build/tcl67/measure)

Variables
  LIBC16_ROOT=DIR     libc16 to use (default build/libc16, the pinned
                      release; or a checkout built with its own make)
  OPT=FLAGS           wcc optimisation flags of build/dev (default -os)
  STACK=SIZE          stack size of build/dev/TCL.EXE (default 32k)
  DOSBOX=PROG         DOSBox-X to run (default dosbox-x)
  DOSBOX_TIMEOUT=N    seconds per DOS test case (default 60)
endef

# The assembled sources are made by one rule; make must not delete them.
.SECONDARY:

.PHONY: all test host test-host libc16-try variants measure check-vendor \
        check-vendor-zip clean help tcl67 test-tcl67 libc16-try-tcl67 \
        variants-tcl67 measure-tcl67 test-suite test-suite-tcl67
