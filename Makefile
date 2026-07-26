CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -lncurses -lm
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: all clean install uninstall check

all: cgalaxy

cgalaxy: cgalaxy.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

check: cgalaxy
	./cgalaxy -h >/dev/null
	./cgalaxy -v
	@if ./cgalaxy -f 2 >/dev/null 2>&1; then \
		echo "expected invalid FPS to fail"; exit 1; \
	fi

install: cgalaxy
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 cgalaxy "$(DESTDIR)$(BINDIR)/cgalaxy"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/cgalaxy"

clean:
	rm -f cgalaxy
