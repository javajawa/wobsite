#!/usr/bin/make -f

.DEFAULT_GOAL = lexer

CFLAGS=-D_POSIX_C_SOURCE=200809L -D_ISOC11_SOURCE -D_FORTIFY_SOURCE=2 -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g -DDEBUG
LDFLAGS= -O0 -g

lexer: test-main.o headers.o
	$(CC) $(LDFLAGS) -o $@ $^

%.h %.c: %.fl
	./lexer-gen.py $<

test-main.o: test-main.c headers.h
	$(CC) $(CFLAGS) -c -o $@ $<
