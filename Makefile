CC ?= cc
PREFIX ?= /usr/local
VERSION ?= 0.3.1

CFLAGS_BASE    := -std=c11 -Wall -Wextra -Wpedantic -Werror
CFLAGS_RELEASE := $(CFLAGS_BASE) -O2
CFLAGS_DEBUG   := $(CFLAGS_BASE) -fsanitize=address,undefined -g -O0
LDFLAGS_DEBUG  := -fsanitize=address,undefined

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    WINDOW_CFLAGS :=
    WINDOW_LIBS   := -framework AppKit -framework Foundation -framework CoreGraphics -framework ApplicationServices -lobjc
else ifneq (,$(findstring MINGW,$(UNAME_S)))
    WINDOW_CFLAGS :=
    WINDOW_LIBS   := -luser32 -lgdi32
else ifneq (,$(findstring MSYS,$(UNAME_S)))
    WINDOW_CFLAGS :=
    WINDOW_LIBS   := -luser32 -lgdi32
else
    HAVE_X11 := $(shell pkg-config --exists x11 2>/dev/null && echo yes || echo no)
    ifeq ($(HAVE_X11),yes)
        WINDOW_CFLAGS := -DSWATCH_HAVE_X11 $(shell pkg-config --cflags x11)
        WINDOW_LIBS   := $(shell pkg-config --libs x11)
    else
        WINDOW_CFLAGS :=
        WINDOW_LIBS   :=
    endif
endif

CFLAGS_RELEASE += $(WINDOW_CFLAGS)
CFLAGS_DEBUG   += $(WINDOW_CFLAGS)

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
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(WINDOW_LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

tests/test_parse: tests/test_parse.c src/parse.c src/parse.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_parse.c src/parse.c

tests/test_render: tests/test_render.c src/render.c src/render.h src/parse.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_render.c src/render.c

tests/test_noise: tests/test_noise.c src/noise.c src/noise.h src/render.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_noise.c src/noise.c

tests/test_window: tests/test_window.c src/window.c src/window.h tests/test.h
	$(CC) $(CFLAGS_DEBUG) $(LDFLAGS_DEBUG) -Isrc -o $@ tests/test_window.c src/window.c

test: tests/test_parse tests/test_render tests/test_noise tests/test_window
	./tests/test_parse
	./tests/test_render
	./tests/test_noise
	./tests/test_window

install: CFLAGS  := $(CFLAGS_RELEASE)
install: LDFLAGS :=
install: swatch
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 ./swatch $(DESTDIR)$(PREFIX)/bin/swatch

clean:
	rm -f ./swatch
	rm -f tests/test_parse tests/test_render tests/test_noise tests/test_window
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
	@echo "test    - Build and run tests/test_parse, tests/test_render, tests/test_noise, tests/test_window under sanitizers"
	@echo "install - Install ./swatch to \$$(PREFIX)/bin/swatch (PREFIX=/usr/local)"
	@echo "clean   - Remove ./swatch, test binaries, *.o, *.dSYM/, dist/"
	@echo "dist    - Produce dist/swatch-\$$(VERSION).tar.gz (VERSION=0.1.0)"
	@echo "help    - Show this help"
