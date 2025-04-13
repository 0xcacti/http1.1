CC = gcc
CFLAGS = -Wall -Wextra -fno-stack-check
# Replace libdill with libmill
LIBMILL_CFLAGS := $(shell pkg-config --cflags libmill 2>/dev/null || echo "-I/usr/local/include")
LIBMILL_LIBS := $(shell pkg-config --libs libmill 2>/dev/null || echo "-L/usr/local/lib -lmill")
CFLAGS += $(LIBMILL_CFLAGS)
LDFLAGS = $(LIBMILL_LIBS)

all: http

http: src/main.c src/http.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f http
