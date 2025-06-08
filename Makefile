CC = gcc
CFLAGS = -Wall -Wextra -fno-stack-check -Iinclude
LIBMILL_CFLAGS := $(shell pkg-config --cflags libmill 2>/dev/null || echo "-I/usr/local/include")
LIBMILL_LIBS := $(shell pkg-config --libs libmill 2>/dev/null || echo "-L/usr/local/lib -lmill")
CRITERION_CFLAGS := $(shell pkg-config --cflags criterion 2>/dev/null || echo "-I/usr/local/include")
CRITERION_LIBS := $(shell pkg-config --libs criterion 2>/dev/null || echo "-L/usr/local/lib -lcriterion")

CFLAGS += $(LIBMILL_CFLAGS)
LDFLAGS = $(LIBMILL_LIBS)
TEST_CFLAGS = $(CFLAGS) $(CRITERION_CFLAGS)
TEST_LIBS = $(CRITERION_LIBS)
REQUEST_SRC = src/internal/request/request.c
HEADERS_SRC = include/internal/headers/headers.h
SOCKET_READER_SRC = src/tcplistener/socket_reader.c

all: tcplistener udpsender

tcplistener: src/tcplistener/main.c $(REQUEST_SRC) $(HEADERS_SRC) $(SOCKET_READER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

udpsender: src/udpsender/main.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_request: tests/internal/request/test_request.c $(REQUEST_SRC) 
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LIBS)

test_headers: tests/internal/headers/test_headers.c $(HEADERS_SRC)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LIBS)

test: test_request
	./test_request
	./test_headers

setup: 
	mkdir -p include/internal/request
	mkdir -p src/internal/request
	mkdir -p tests/internal/request
	mkdir -p include/internal/headers

clean:
	rm -f tcplistener udpsender test_request test_headers

.PHONY: all test setup clean
