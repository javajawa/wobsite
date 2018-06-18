#!/usr/bin/make -f

WOBSITE := wobsite

.DEFAULT_GOAL = build
.PHONY = build debug clean

CFLAGS=-D_POSIX_C_SOURCE=200809L -D_ISOC11_SOURCE -D_FORTIFY_SOURCE=2 -std=c11 -Wall -Wextra -Werror -pedantic -Isrc -Igen
LDFLAGS=-lpthread

LAYOUTS   = $(addprefix gen/,$(wildcard layouts/*.xml))
LAYOUTS_C = $(patsubst %.xml,%.c,$(LAYOUTS))
LAYOUTS_H = $(patsubst %.xml,%.h,$(LAYOUTS))

LAYOUTSO  = $(addprefix build/,$(wildcard layouts/*.xml))
LAYOUTS_O = $(patsubst %.xml,%.o,$(LAYOUTSO))

DAEMON_O := build/daemon/wobsite.o build/daemon/threading.o build/daemon/responder.o
RENDER_O := build/renderer/render.o
COMMON_O := build/logging.o build/string/strsep.o

GLOBAL_H := src/config.h src/layout.h src/logging.h src/daemon/globals.h

build: CFLAGS  += -O3 -s
build: LDFLAGS += -O3 -s
build: $(WOBSITE)

debug: CFLAGS  += -O0 -g -DDEBUG
debug: LDFLAGS += -O0 -g
debug: $(WOBSITE)

$(WOBSITE): $(COMMON_O) $(RENDER_O) $(DAEMON_O) $(LAYOUTS_O)
	$(CC) $(LDFLAGS) -o $@ $^

gen/layouts/%.c gen/layouts/%.h: layouts/%.xml
	-mkdir -vp $(dir $@)
	./parse_layout $<

build/%.o : src/%.c $(GLOBAL_H)
	-mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o : gen/%.c gen/%.h $(GLOBAL_H)
	-mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -Rf build $(LAYOUTS_C) $(LAYOUTS_H) $(WOBSITE)
	-rmdir gen/layouts gen

