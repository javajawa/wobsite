#!/usr/bin/make -f

WOBSITE := wobsite

.DEFAULT_GOAL = build
.PHONY = build debug clean

MAKEFLAGS += --no-print-directory

SRC     := src
INCLUDE := include
GEN     := gen
BUILD   := build

LAYOUT  := layouts

CFLAGS  := -D_POSIX_C_SOURCE=200809L -D_ISOC11_SOURCE -D_FORTIFY_SOURCE=2
CFLAGS  += -std=c11 -Wall -Wextra -Werror -pedantic -I$(INCLUDE)
LDFLAGS := -lpthread

LAYOUTS   = $(addprefix $(GEN)/,$(wildcard $(LAYOUT)/*.xml))
LAYOUTS_C = $(patsubst %.xml,%.c,$(LAYOUTS))
LAYOUTS_H = $(addprefix $(INCLUDE)/,$(patsubst %.xml,%.h,$(LAYOUTS)))

LAYOUTSO  = $(addprefix $(BUILD)/,$(wildcard layouts/*.xml))
LAYOUTS_O = $(patsubst %.xml,%.o,$(LAYOUTSO))

DAEMON_O := $(addprefix $(BUILD)/daemon/,wobsite.o threading.o responder.o)
RENDER_O := $(BUILD)/renderer/render.o
COMMON_O := $(BUILD)/logging.o $(BUILD)/string/strsep.o

GLOBAL_H := include/config.h include/layout.h include/logging.h

build: CFLAGS  += -O3 -s
build: LDFLAGS += -O3 -s
build: $(WOBSITE)

debug: CFLAGS  += -O0 -g -DDEBUG
debug: LDFLAGS += -O0 -g
debug: $(WOBSITE)

$(WOBSITE): $(COMMON_O) $(RENDER_O) $(DAEMON_O) $(LAYOUTS_O)
	$(CC) $(LDFLAGS) -o $@ $^

$(GEN)/$(LAYOUT)/%.c: $(LAYOUT)/%.xml
	@mkdir -vp $(dir $@)
	@$(MAKE) $(MAKEFLAGS) "$(INCLUDE)/$(patsubst %.c,%.h,$(@))"

$(INCLUDE)/$(GEN)/$(LAYOUTS)/%.h: $(LAYOUT)/%.xml
	@mkdir -vp $(dir $@)
	tools/parse_layout $<

$(BUILD)/%.o : $(SRC)/%.c $(GLOBAL_H)
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o : $(GEN)/%.c $(INCLUDE)/$(GEN)/%.h $(GLOBAL_H)
	@mkdir -vp $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -Rf build $(LAYOUTS_C) $(LAYOUTS_H) $(WOBSITE)
	-rmdir $(GEN)/$(LAYOUTS) $(GEN)
	-rmdir $(addprefix $(INCLUDE),$(GEN)/$(LAYOUTS) $(GEN))
