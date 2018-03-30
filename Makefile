#!/usr/bin/make -f

WOBSITE := wobsite

.DEFAULT_GOAL = all
.PHONY = all clean
.PRECIOUS = layouts/%.c

CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -Isrc -Igen -g -O0
LDFLAGS=-g -O0 -lpthread

LAYOUTS   = $(addprefix gen/,$(wildcard layouts/*.xml))
LAYOUTS_C = $(patsubst %.xml,%.c,$(LAYOUTS))
LAYOUTS_H = $(patsubst %.xml,%.h,$(LAYOUTS))

LAYOUTSO  = $(addprefix build/,$(wildcard layouts/*.xml))
LAYOUTS_O = $(patsubst %.xml,%.o,$(LAYOUTSO))

DAEMON_O := build/daemon/wobsite.o build/daemon/threading.o build/daemon/responder.o
RENDER_O := build/renderer/render.o
COMMON_O :=

GLOBAL_H := src/globals.h src/layout.h

$(warning $(LAYOUTS_O))
$(warning $(LAYOUTS_C))
$(warning $(LAYOUTS_H))

all: $(WOBSITE)

$(WOBSITE): $(COMMON_O) $(RENDER_O) $(DAEMON_O) $(LAYOUTS_O)
	$(CC) $(LDFLAGS) -o $@ $^

gen/layouts/%.c gen/layouts/%.h: layouts/%.xml
	-mkdir -vp $(dir $@)
	./parse_layout $<

build/%.o : src/%.c
	mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -Rf build $(LAYOUTS_C) $(LAYOUTS_H) $(WOBSITE)

