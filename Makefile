CC = gcc
CFLAGS = -Wall -Wextra 
LIBDILL_CFLAGS := $(shell pkg-config --cflags libdill)
LIBDILL_LIBS := $(shell pkg-config --libs libdill)

CFLAGS += $(LIBDILL_CFLAGS)
LDFLAGS = $(LIBDILL_LIBS)

all: http

http: src/main.c src/http.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean: 
	rm -f http
