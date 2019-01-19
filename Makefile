include config.mak

OBJS=resdet.o image.o methods.o image/pgm.o
LIB=lib/libresdet.a
TOOLS=resdet stat profile imgread

EXTRAFLAGS=
ifdef HAVE_FFTW
	OBJS += transform/fftw.o
else
	OBJS += transform/kiss_fft.o
	EXTRAFLAGS += -Ilib/kissfft -Ilib/kissfft/tools
	OBJS += $(addprefix kissfft/, kiss_fft.o $(addprefix tools/, kiss_fftnd.o kiss_fftndr.o kiss_fftr.o))
endif

ifdef DEFAULT_RANGE
	EXTRAFLAGS += -DDEFAULT_RANGE=$(DEFAULT_RANGE)
endif
ifdef COEFF_PRECISION
	EXTRAFLAGS += -DCOEFF_PRECISION=$(COEFF_PRECISION)
endif
ifdef INTER_PRECISION
	EXTRAFLAGS += -DINTER_PRECISION=$(INTER_PRECISION)
endif
ifdef PIXEL_MAX
	EXTRAFLAGS += -DPIXEL_MAX=$(PIXEL_MAX)
endif

ifdef HAVE_LIBJPEG
	OBJS += image/libjpeg.o
endif
ifdef HAVE_LIBPNG
	OBJS += image/libpng.o
endif
ifdef HAVE_MJPEGTOOLS
	OBJS += image/mjpegtools.o
endif
ifdef HAVE_MAGICKWAND
	OBJS += image/magickwand.o
endif

OBJS := $(addprefix lib/, $(OBJS))

CFLAGS := -Iinclude/ -Ilib/ $(DEFS) $(EXTRAFLAGS) $(CFLAGS)

all: $(TOOLS)

$(LIB): $(OBJS)
	$(AR) rcs $@ $+

resdet: src/resdet.o $(LIB)
	$(CC) -o $@ $(DEFS) $+ $(LIBS)

profile: src/profile.o $(LIB)
	$(CC) -o $@ $(DEFS) $+ $(LIBS)

stat: src/stat.o $(LIB)
	$(CC) -o $@ $(DEFS) $+ $(LIBS)

imgread: src/imgread.o $(LIB)
	$(CC) -o $@ $(DEFS) $+ $(LIBS)

install-lib: $(LIB)
	install include/resdet.h $(INCPREFIX)/
	install $(LIB) $(LIBPREFIX)/
ifneq ($(PCPREFIX),)
	mkdir -p $(PCPREFIX)
	install lib/resdet.pc $(PCPREFIX)/
endif

install: resdet
	install resdet $(BINPREFIX)/

uninstall:
	rm -f $(BINPREFIX)/resdet $(LIBPREFIX)/libresdet.a $(PCPREFIX)/resdet.pc $(INCPREFIX)/resdet.h
	
clean:
	rm -f -- src/*.o $(OBJS) $(LIB) $(TOOLS)

.PHONY: all install uninstall clean
