CC = gcc
CFLAGS = -Wall -Wextra -fno-stack-check
# Replace libdill with libmill
LIBMILL_CFLAGS := $(shell pkg-config --cflags libmill 2>/dev/null || echo "-I/usr/local/include")
LIBMILL_LIBS := $(shell pkg-config --libs libmill 2>/dev/null || echo "-L/usr/local/lib -lmill")
CFLAGS += $(LIBMILL_CFLAGS)
LDFLAGS = $(LIBMILL_LIBS)

all: tcplistener udpsender

tcplistener: src/tcplistener/main.c src/tcplistener/http.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

udpsender: src/udpsender/main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f tcplistener udpsender

.PHONY: all clean
