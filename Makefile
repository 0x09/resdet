include config.mak

OBJS=resdetect.o analysis.o util.o image.o image_readers.o methods.o
LIB=lib/libresdet.a

ifdef SHARED
	SHAREDLIB=libresdet.$(SOEXT)
endif

EXTRAFLAGS=
ifdef HAVE_FFTW
	OBJS += transform/fftw.o
else
	OBJS += transform/kiss_fft.o
	CFLAGS_LIB += -Ilib/kissfft
	OBJS += $(addprefix kissfft/, kiss_fft.o kiss_fftnd.o kiss_fftndr.o kiss_fftr.o)
endif

ifdef DEFAULT_RANGE
	CFLAGS_LIB += -DDEFAULT_RANGE=$(DEFAULT_RANGE)
endif
ifdef COEFF_PRECISION
	CFLAGS_LIB += -DCOEFF_PRECISION=$(COEFF_PRECISION)
endif
ifdef INTER_PRECISION
	CFLAGS_LIB += -DINTER_PRECISION=$(INTER_PRECISION)
endif
ifdef PIXEL_MAX
	CFLAGS_LIB += -DPIXEL_MAX=$(PIXEL_MAX)
endif

ifdef HAVE_LIBJPEG
	OBJS += image/libjpeg.o
endif
ifdef HAVE_LIBPNG
	OBJS += image/libpng.o
endif
ifdef HAVE_MAGICKWAND
	OBJS += image/magickwand.o
endif
ifdef HAVE_FFMPEG
	OBJS += image/ffmpeg.o
endif

ifndef OMIT_PGM_READER
	OBJS += image/pgm.o
endif
ifndef OMIT_PFM_READER
	OBJS += image/pfm.o
endif
ifndef OMIT_Y4M_READER
	OBJS += image/y4m.o
endif

OBJS := $(addprefix lib/, $(OBJS))

DEPS := $(OBJS:.o=.d) $(addprefix src/, $(addsuffix .d, $(TOOLS))) test/lib/main.d
CPPFLAGS += -MMD -MP

all: $(SHAREDLIB) $(TOOLS)

CFLAGS_LIB := -Iinclude/ -Ilib/ $(DEFS) $(CFLAGS_LIB) $(CFLAGS)

lib/transform/fftw.o:   CFLAGS := $(CFLAGS_FFTW) $(CFLAGS_LIB)
lib/image/libjpeg.o:    CFLAGS := $(CFLAGS_libjpeg) $(CFLAGS_LIB)
lib/image/libpng.o:     CFLAGS := $(CFLAGS_libpng) $(CFLAGS_LIB)
lib/image/magickwand.o: CFLAGS := $(CFLAGS_MagickWand) $(CFLAGS_LIB)
lib/image/ffmpeg.o:     CFLAGS := $(CFLAGS_ffmpeg) $(CFLAGS_LIB)

$(LIB): CFLAGS := $(CFLAGS_LIB)
$(LIB): $(OBJS)
	$(AR) rcs $@ $+

ifdef SHARED
VERSION=$(shell pkg-config --modversion lib/resdet.pc | cut -d+ -f1)
VERSION_MAJOR_MINOR=$(shell pkg-config --modversion lib/resdet.pc | cut -d. -f1-2)
VERSION_MAJOR=$(shell pkg-config --modversion lib/resdet.pc | cut -d. -f1)

ifeq ($(SOEXT),dylib)
$(SHAREDLIB): LDFLAGS += -dynamiclib -install_name $(LIBPREFIX)/$(SHAREDLIB) -Wl,-current_version,$(VERSION) -Wl,-compatibility_version,$(VERSION_MAJOR)
else
$(SHAREDLIB): LDFLAGS += -shared
ifeq ($(SOEXT),so)
$(SHAREDLIB): LDFLAGS += -Wl,-soname,$(SHAREDLIB).$(VERSION_MAJOR)
endif
endif

$(SHAREDLIB): CFLAGS := $(CFLAGS_LIB)
$(SHAREDLIB): $(OBJS)
	$(CC) -o $@ $+ $(LDFLAGS) $(LDLIBS)
endif

vpath %.o src

CFLAGS := -Iinclude/ $(CLIDEFS) $(CFLAGS_CLI) $(CFLAGS)

resdet:  src/resdet.o $(LIB)
profile: src/profile.o $(LIB)
stat:    src/stat.o $(LIB)
imgread: src/imgread.o $(LIB)

lib: $(LIB) $(SHAREDLIB)

install-lib: lib
	install -m644 include/resdet.h $(INCPREFIX)/
	install -m644 $(LIB) $(LIBPREFIX)/
ifdef SHARED
ifeq ($(SOEXT),dylib)
	install -m644 $(SHAREDLIB) $(LIBPREFIX)/libresdet.$(VERSION).dylib
	ln -fs $(LIBPREFIX)/libresdet.$(VERSION).dylib $(LIBPREFIX)/libresdet.$(VERSION_MAJOR_MINOR).dylib
	ln -fs $(LIBPREFIX)/libresdet.$(VERSION).dylib $(LIBPREFIX)/libresdet.$(VERSION_MAJOR).dylib
	ln -fs $(LIBPREFIX)/libresdet.$(VERSION).dylib $(LIBPREFIX)/libresdet.dylib
else ifeq ($(SOEXT),so)
	install -m644 $(SHAREDLIB) $(LIBPREFIX)/libresdet.so.$(VERSION)
	ln -fs $(LIBPREFIX)/libresdet.so.$(VERSION) $(LIBPREFIX)/libresdet.so.$(VERSION_MAJOR)
	ln -fs $(LIBPREFIX)/libresdet.so.$(VERSION) $(LIBPREFIX)/libresdet.so
else
	install -m644 $(SHAREDLIB) $(LIBPREFIX)/
endif
endif
ifneq ($(PCPREFIX),)
	mkdir -p $(PCPREFIX)
	install -m644 lib/resdet.pc $(PCPREFIX)/
endif

install: resdet
	install resdet $(BINPREFIX)/

uninstall-lib:
	$(RM) $(BINPREFIX)/resdet $(LIBPREFIX)/libresdet.a $(PCPREFIX)/resdet.pc $(INCPREFIX)/resdet.h
	$(RM) $(LIBPREFIX)/libresdet.so.$(VERSION) $(LIBPREFIX)/libresdet.so.$(VERSION_MAJOR) $(LIBPREFIX)/libresdet.so $(LIBPREFIX)/libresdet.$(VERSION_MINOR).dylib $(LIBPREFIX)/libresdet.$(VERSION_MAJOR).dylib $(LIBPREFIX)/libresdet.dylib

uninstall: uninstall-lib
	$(RM) $(BINPREFIX)/resdet

TESTSRCS = $(wildcard test/lib/test_*.c)
TESTOBJS = $(TESTSRCS:%.c=%.o)
DEPS += $(TESTSRCS:%.c=%.d)

test/lib/tests_main.c: test/gen_tests.awk $(TESTSRCS)
	$(AWK) -f $+ > test/lib/tests_main.c

test/lib/tests.o: test/lib/tests_main.c

test_libresdet: CFLAGS := -Iinclude $(DEFS) -DRESDET_EXPORT -DRESDET_LIBVERSION=\"$(shell pkg-config --modversion lib/resdet.pc)\" $(TEST_CFLAGS)
test_libresdet: LDLIBS := $(LDLIBS) $(TEST_LIBS)
test_libresdet: test/lib/tests.o $(TESTOBJS) $(LIB)
	$(CC) $(LDFLAGS) -o $@ test/lib/tests.o $(TESTOBJS) $(LIB) $(LDLIBS)

check_lib: test_libresdet
	@echo "Testing libresdet"
	@./test_libresdet

check_resdet: resdet
	@echo "Testing resdet"
	@PATH="$$PWD:$$PATH" bash_unit test/bin/test_resdet.sh

ifndef SHARED
ifneq ($(MAKECMDGOALS),)
ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),check check_python_bindings))
$(error the shared library is required for make check_python_bindings, use ./configure --enable-shared)
endif
endif
endif

.testvenv:
	$(PYTHON) -m venv .testvenv
	. .testvenv/bin/activate; pip install -r test/bindings/python/requirements.txt

check_python_bindings: .testvenv $(SHAREDLIB)
	@echo "Testing Python bindings"
	@. .testvenv/bin/activate; LD_LIBRARY_PATH=. PYTHONPATH="$$PWD/bindings/python" pytest -v

check_emscripten_bindings: libresdet.mjs
	@cd test/bindings/emscripten; CI=true npm run test

check: check_lib check_resdet check_python_bindings

vpath libresdet.mjs bindings/emscripten/

libresdet.mjs: $(LIB)
	emcc -o bindings/emscripten/$@ $^ -s ALLOW_MEMORY_GROWTH=1 -s "EXPORTED_FUNCTIONS=@$$PWD/bindings/emscripten/exported_functions.txt" -s EXPORTED_RUNTIME_METHODS=cwrap,HEAPF32,getValue,UTF8ToString

clean:
	$(RM) src/*.o $(OBJS) $(LIB) $(TOOLS) $(DEPS) $(SHAREDLIB) test_libresdet test/lib/tests.o test/lib/tests_main.c $(TESTOBJS) bindings/emscripten/libresdet.{mjs,wasm}

.PHONY: all lib install install-lib uninstall-lib uninstall check_lib check_resdet check_python_bindings check clean

-include $(DEPS)
