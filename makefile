CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
PREFIX  ?= /usr/local
BINDIR  := $(PREFIX)/bin

SRC     := src/euniq.c
TARGET  := euniq

.PHONY: all debug test install uninstall clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

debug: $(SRC)
	$(CC) -std=c11 -g -O1 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -o $(TARGET) $<

test: $(TARGET)
	@chmod +x tests/run_tests.sh
	@./tests/run_tests.sh

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
