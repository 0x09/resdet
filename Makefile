include config.mak

LIB=lib/libresdet.a
TOOLS=$(addprefix tools/, resdet stat profile)

EXTRAFLAGS=
ifneq ($(SHAREPREFIX),)
	EXTRAFLAGS += -DRSRC_DIR='"$(SHAREPREFIX)"'
endif
ifdef DEFAULT_RANGE
	EXTRAFLAGS += -DDEFAULT_RANGE=$(DEFAULT_RANGE)
endif

CFLAGS := -Iinclude/ -Ilib/ $(DEFS) $(EXTRAFLAGS) $(CFLAGS)

all: $(TOOLS)

$(LIB): $(addprefix lib/, resdet.o image.o methods.o)
	$(AR) rcs $@ $+

tools/resdet: src/resdet.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

tools/profile: src/profile.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

tools/stat: src/stat.o $(LIB)
	$(CC) -o $@ $(LIBS) $+

install-lib: $(LIB)
	install include/resdet.h $(INCPREFIX)/
	install $(LIB) $(LIBPREFIX)/

install: tools/resdet
	install tools/resdet $(BINPREFIX)/
ifneq ($(SHAREPREFIX),)
	mkdir -p $(SHAREPREFIX)
	install share/magic $(SHAREPREFIX)/
endif

uninstall:
	rm -f $(BINPREFIX)/resdet
	rm -f $(LIBPREFIX)/libresdet.a
	rm -f $(INCPREFIX)/resdet.h
	rm -f $(SHAREPREFIX)/magic
	rmdir $(SHAREPREFIX) &> /dev/null
	
clean:
	rm -f -- src/*.o lib/*.o $(LIB) $(TOOLS)

.PHONY: all install uninstall clean