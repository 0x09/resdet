include config.mak

OBJS=resdet.o image.o methods.o image/y4m.o
LIB=lib/libresdet.a

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

ifndef OMIT_NATIVE_PGM_READER
	OBJS += image/pgm.o image/pfm.o
endif

OBJS := $(addprefix lib/, $(OBJS))

DEPS := $(OBJS:.o=.d) $(src/%.c:.c=.d)
CPPFLAGS += -MMD -MP

all: $(TOOLS)

CFLAGS_LIB := -Iinclude/ -Ilib/ $(DEFS) $(CFLAGS_LIB) $(CFLAGS)

lib/transform/fftw.o:   CFLAGS := $(CFLAGS_FFTW) $(CFLAGS_LIB)
lib/image/libjpeg.o:    CFLAGS := $(CFLAGS_libjpeg) $(CFLAGS_LIB)
lib/image/libpng.o:     CFLAGS := $(CFLAGS_libpng) $(CFLAGS_LIB)
lib/image/magickwand.o: CFLAGS := $(CFLAGS_MagickWand) $(CFLAGS_LIB)

$(LIB): CFLAGS := $(CFLAGS_LIB)
$(LIB): $(OBJS)
	$(AR) rcs $@ $+

vpath %.o src

CFLAGS := -Iinclude/ $(CLIDEFS) $(CFLAGS_CLI) $(CFLAGS)

resdet:  src/resdet.o $(LIB)
profile: src/profile.o $(LIB)
stat:    src/stat.o $(LIB)
imgread: src/imgread.o $(LIB)

install-lib: $(LIB)
	install -m644 include/resdet.h $(INCPREFIX)/
	install -m644 $(LIB) $(LIBPREFIX)/
ifneq ($(PCPREFIX),)
	mkdir -p $(PCPREFIX)
	install -m644 lib/resdet.pc $(PCPREFIX)/
endif

install: resdet
	install resdet $(BINPREFIX)/

uninstall:
	$(RM) $(BINPREFIX)/resdet $(LIBPREFIX)/libresdet.a $(PCPREFIX)/resdet.pc $(INCPREFIX)/resdet.h
	
clean:
	$(RM) src/*.o $(OBJS) $(LIB) $(TOOLS) $(DEPS)

.PHONY: all install uninstall clean

-include $(DEPS)
