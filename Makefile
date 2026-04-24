CC ?= cc
PREFIX ?= /usr/local
VERSION ?= 0.1.0

CFLAGS_BASE    := -std=c11 -Wall -Wextra -Wpedantic -Werror
CFLAGS_RELEASE := $(CFLAGS_BASE) -O2
CFLAGS_DEBUG   := $(CFLAGS_BASE) -fsanitize=address,undefined -g -O0
LDFLAGS_DEBUG  := -fsanitize=address,undefined

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

.PHONY: all debug test install clean dist help

all: CFLAGS  := $(CFLAGS_RELEASE)
all: LDFLAGS :=
all: swatch

debug: CFLAGS  := $(CFLAGS_DEBUG)
debug: LDFLAGS := $(LDFLAGS_DEBUG)
debug: swatch

swatch: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

tests/test_parse: tests/test_parse.c src/parse.c src/parse.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_parse.c src/parse.c

tests/test_render: tests/test_render.c src/render.c src/render.h src/parse.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_render.c src/render.c

test: tests/test_parse tests/test_render
	./tests/test_parse
	./tests/test_render

install: CFLAGS  := $(CFLAGS_RELEASE)
install: LDFLAGS :=
install: swatch
	install -m 755 -D ./swatch $(PREFIX)/bin/swatch

clean:
	rm -f ./swatch
	rm -f tests/test_parse tests/test_render
	find . -name '*.o' -type f -delete
	find . -name '*.dSYM' -type d -prune -exec rm -rf {} +
	rm -rf dist/

dist:
	mkdir -p dist
	rm -rf dist/swatch-$(VERSION)
	mkdir -p dist/swatch-$(VERSION)
	cp Makefile README.md idea.md dist/swatch-$(VERSION)/
	[ -f LICENSE ] && cp LICENSE dist/swatch-$(VERSION)/ || true
	cp -r src tests dist/swatch-$(VERSION)/
	tar czf dist/swatch-$(VERSION).tar.gz -C dist swatch-$(VERSION)
	rm -rf dist/swatch-$(VERSION)

help:
	@echo "all     - Build ./swatch (release, -O2, -Werror) [default]"
	@echo "debug   - Build ./swatch with address+undefined sanitizers"
	@echo "test    - Build and run tests/test_parse and tests/test_render under sanitizers"
	@echo "install - Install ./swatch to \$$(PREFIX)/bin/swatch (PREFIX=/usr/local)"
	@echo "clean   - Remove ./swatch, test binaries, *.o, *.dSYM/, dist/"
	@echo "dist    - Produce dist/swatch-\$$(VERSION).tar.gz (VERSION=0.1.0)"
	@echo "help    - Show this help"
