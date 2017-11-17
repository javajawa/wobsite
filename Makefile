TARGET = web-example

.DEFAULT_GOAL = all
.PHONY = all clean
.PRECIOUS = layouts/%.c

CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic -I. -g -O0
LDFLAGS=-g -O0 -lfcgi -lpthread

GENERAL_O=render.o web-example.o
LAYOUTS_O=$(patsubst %.c,%.o,$(wildcard layouts/*.c))

all: $(TARGET) wobsite

$(TARGET): $(GENERAL_O) $(LAYOUTS_O)
wobsite: wobsite.o responder.o threading.o

layouts/%.c layouts/%.h: layouts/%.xml
	./parse_layout $<

clean:
	rm -f $(GENERAL_O) $(LAYOUTS_O) $(TARGET)

