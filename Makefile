CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
SRC = src/buf.c src/util.c src/markdown.c

.PHONY: all site test clean

all: sitegen

sitegen: src/sitegen.c $(SRC) src/buf.h src/util.h src/markdown.h
	$(CC) $(CFLAGS) -o $@ src/sitegen.c $(SRC)

test_markdown: src/test_markdown.c $(SRC) src/buf.h src/util.h src/markdown.h
	$(CC) $(CFLAGS) -o $@ src/test_markdown.c $(SRC)

site: sitegen
	./sitegen

test: test_markdown sitegen
	./test_markdown
	./sitegen

clean:
	rm -f sitegen test_markdown
