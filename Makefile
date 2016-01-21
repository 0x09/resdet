include config.mak

LIB=lib/libresdet.a
TOOLS=resdet stat profile

EXTRAFLAGS=
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

$(LIB): $(addprefix lib/, resdet.o image.o methods.o)
	$(AR) rcs $@ $+

resdet: src/resdet.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

profile: src/profile.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

stat: src/stat.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

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
	rm -f $(BINPREFIX)/resdet
	rm -f $(LIBPREFIX)/libresdet.a
	rm -f $(PCPREFIX)/resdet.pc
	rm -f $(INCPREFIX)/resdet.h
	rm -f $(SHAREPREFIX)/magic
	rmdir $(SHAREPREFIX) &> /dev/null
	
clean:
	rm -f -- src/*.o lib/*.o $(LIB) $(TOOLS)

.PHONY: all install uninstall clean
