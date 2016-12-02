include config.mak

OBJS=resdet.o image.o methods.o transform.o
LIB=lib/libresdet.a
TOOLS=resdet stat profile

EXTRAFLAGS=
ifndef HAVE_FFTW
	EXTRAFLAGS += -Ilib/kissfft -Ilib/kissfft/tools
	OBJS += $(addprefix kissfft/, kiss_fft.o $(addprefix tools/, kiss_fftnd.o kiss_fftndr.o kiss_fftr.o))
endif

OBJS := $(addprefix lib/, $(OBJS))

ifneq ($(SHAREPREFIX),)
	EXTRAFLAGS += -DRSRC_DIR='"$(SHAREPREFIX)"'
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

install-lib: $(LIB)
	install include/resdet.h $(INCPREFIX)/
	install $(LIB) $(LIBPREFIX)/
ifneq ($(PCPREFIX),)
	mkdir -p $(PCPREFIX)
	install lib/resdet.pc $(PCPREFIX)/
endif

install: resdet
	install resdet $(BINPREFIX)/
ifneq ($(SHAREPREFIX),)
	mkdir -p $(SHAREPREFIX)
	install share/magic $(SHAREPREFIX)/
endif

uninstall:
	rm -f $(BINPREFIX)/resdet $(LIBPREFIX)/libresdet.a $(PCPREFIX)/resdet.pc $(INCPREFIX)/resdet.h $(SHAREPREFIX)/magic
	rmdir $(SHAREPREFIX) &> /dev/null
	
clean:
	rm -f -- src/*.o $(OBJS) $(LIB) $(TOOLS)

.PHONY: all install uninstall clean
