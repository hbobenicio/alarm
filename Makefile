# SRC = src/config.c src/main.c
OBJS = config.o main.o # $(SRCS:.c=.o)
BIN = alarm

PKG_CONFIG_CFLAGS  := $(shell pkg-config --cflags gtk+-3.0)
PKG_CONFIG_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)

LIBS = -pthread -lasound

CFLAGS  = -Wall -Wextra -pedantic -std=c11 $(PKG_CONFIG_CFLAGS)
LDFLAGS = -Wall -Wextra -pedantic -std=c11 $(PKG_CONFIG_LDFLAGS) $(LIBS)

.PHONY: all release debug vscode-include-paths distclean clean

all: $(BIN)

release: CFLAGS += -O3
release: LDFLAGS += -O3
release: all
	strip $(BIN)

debug: CFLAGS += -g -fsanitize=address -O0
debug: LDFLAGS += -g -fsanitize=address -O0
debug: all

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

vscode-include-paths:
	pkg-config --cflags-only-I gtk+-3.0 | sed 's/-I\([^ ]*\)/"\1",\n/g'

distclean: clean
	rm -f $(BIN)

clean:
	rm -f *.o
