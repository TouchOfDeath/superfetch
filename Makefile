CC = cc
CFLAGS = -O3 -march=native -flto -Wall -Wextra -pedantic
LDFLAGS = -flto
LIBS = -lm -lpthread

DESTDIR ?= 
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1
BASHCOMPDIR = /etc/bash_completion.d

all: superfetch

# Build with images (default)
superfetch: superfetch.c config.h logos.h stb_image.h
	$(CC) $(CFLAGS) -DENABLE_IMAGES $(LDFLAGS) -o superfetch superfetch.c $(LIBS)

# Build without images (tiny binary)
pure:
	$(CC) $(CFLAGS) $(LDFLAGS) -o superfetch superfetch.c $(LIBS)

install: superfetch
	install -D -m 755 superfetch $(DESTDIR)$(BINDIR)/superfetch
	install -D -m 644 superfetch.1 $(DESTDIR)$(MANDIR)/superfetch.1
	# Try to install bash completion locally if /etc/ is not writable without sudo,
	# but generally 'sudo make install' handles /etc
	@if [ -w $(DESTDIR)$(BASHCOMPDIR) ] || [ "$$(id -u)" = "0" ]; then \
		install -D -m 644 superfetch-completion.bash $(DESTDIR)$(BASHCOMPDIR)/superfetch; \
	else \
		echo "Warning: Cannot install bash completion to $(BASHCOMPDIR) without sudo. Skipping."; \
	fi

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/superfetch
	rm -f $(DESTDIR)$(MANDIR)/superfetch.1
	rm -f $(DESTDIR)$(BASHCOMPDIR)/superfetch

clean:
	rm -f superfetch *.o

.PHONY: all pure install uninstall clean
