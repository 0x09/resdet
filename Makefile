include config.mak

OBJS=resdetect.o analysis.o util.o image.o methods.o image/y4m.o
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

ifndef OMIT_NATIVE_PGM_PFM_READERS
	OBJS += image/pgm.o image/pfm.o
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

test/lib/main.c: test/gen_tests.awk $(TESTSRCS)
	$(AWK) -f $+ > test/lib/main.c

test_libresdet: CFLAGS := -Iinclude $(DEFS) -DRESDET_EXPORT -DRESDET_LIBVERSION=\"$(shell pkg-config --modversion lib/resdet.pc)\" $(TEST_CFLAGS)
test_libresdet: LDLIBS := $(LDLIBS) $(TEST_LIBS)
test_libresdet: test/lib/main.o $(TESTOBJS) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $+ $(LDLIBS)

check: test_libresdet resdet
	@./test_libresdet
	PATH="$$PWD:$$PATH" bash_unit test/bin/test_resdet.sh

clean:
	$(RM) src/*.o $(OBJS) $(LIB) $(TOOLS) $(DEPS) $(SHAREDLIB) test_libresdet test/lib/main.* $(TESTOBJS)

.PHONY: all lib install install-lib uninstall-lib uninstall check clean

-include $(DEPS)
