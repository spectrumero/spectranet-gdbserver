ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
GDBSERVER_C_SOURCES=$(wildcard src/*.c)
GDBSERVER_C_OBJECTS=$(GDBSERVER_C_SOURCES:.c=_c.o)
GDBSERVER_ASM_SOURCES=$(wildcard src/*.asm)
GDBSERVER_ASM_OBJECTS=$(GDBSERVER_ASM_SOURCES:.asm=_asm.o)
INCLUDES=-I$(ROOT_DIR)/include/spectranet -I$(ROOT_DIR)/src
JUST_PRINT:=$(findstring n,$(MAKEFLAGS))

ifneq (,$(JUST_PRINT))
	PHONY_OBJS := yes
	CC = gcc
	LD = ar
	FAKE_DEFINES = -D__LIB__="" -D__CALLEE__="" -D__FASTCALL__=""
	FAKE_INCLUDES = -I/usr/local/share/z88dk/include
	CFLAGS = $(FAKE_DEFINES) -nostdinc $(INCLUDES) $(FAKE_INCLUDES)
	GDBSERVER_FLAGS = -o build/gdbserver
else
	CC = zcc
	LD = zcc
	DEBUG ?= -debug
	CFLAGS = +zx $(DEBUG) $(INCLUDES)
	LINK_FLAGS = -L$(ROOT_DIR)/libs -llibs/libsocket_np.lib -llibs/libspectranet_np.lib
	BIN_FLAGS = -startup=31 --no-crt -subtype=bin
	PRG_OBJECTS = src/prg/prg.c
	LDFLAGS = +zx $(DEBUG) $(LINK_FLAGS)
	GDBSERVER_FLAGS = -create-app
endif

all: gdbserver

build/gdbserver: $(GDBSERVER_C_OBJECTS) $(GDBSERVER_ASM_OBJECTS)
	$(LD) $(LDFLAGS) $(BIN_FLAGS) -o build/gdbserver $(GDBSERVER_FLAGS) $(GDBSERVER_C_OBJECTS) $(GDBSERVER_ASM_OBJECTS)

build:
	mkdir -p $@

include/spectranet:
	@mkdir -p include/spectranet

libs:
	mkdir -p $@

libs/libsocket_np.lib: libs include/spectranet
	make DESTLIBS=$(ROOT_DIR)/libs DESTINCLUDE=$(ROOT_DIR)/include/spectranet SRCGEN=$(ROOT_DIR)/spectranet/scripts/makesources.pl -C spectranet/socklib nplib install

libs/libspectranet_np.lib: libs include/spectranet
	make DESTLIBS=$(ROOT_DIR)/libs DESTINCLUDE=$(ROOT_DIR)/include/spectranet SRCGEN=$(ROOT_DIR)/spectranet/scripts/makesources.pl -C spectranet/libspectranet nplib install

spectranet-libs: libs/libsocket_np.lib libs/libspectranet_np.lib

gdbserver: build spectranet-libs build/gdbserver

%_c.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%_asm.o: %.asm
	$(CC) $(CFLAGS) -o $@ -c $<

get-size:
	@cat build/gdbserver.map | sed -n "s/^\\([a-zA-Z0-9_]*\\).*= .\([A-Z0-9]*\) ; \([^,]*\), .*/\2,\1,\3/p" | sort | python3 tools/symbol_sizes.py

deploy:
	ethup 127.0.0.1 build/gdbserver

clean:
	@rm -rf build/*
	@rm -f src/*.o
	@rm -f libs/*
	@rm -f spectranet/libspectranet/*.o
	@rm -f spectranet/socklib/*.o

.PHONY: clean get-size deploy

ifeq ($(PHONY_OBJS),yes)
.PHONY: $(GDBSERVER_SOURCES)
.PHONY: spectranet-libs libs/libsocket_np.lib libs/libspectranet_np.lib
endif