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
HEADERS_SRC = src/internal/headers/headers.c
SERVER_SRC = src/internal/server/server.c
RESPONSE_SRC = src/internal/response/response.c
SOCKET_READER_SRC = src/internal/socket_reader/socket_reader.c

all: tcplistener udpsender httpserver

tcplistener: src/tcplistener/main.c $(REQUEST_SRC) $(HEADERS_SRC) $(SOCKET_READER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

udpsender: src/udpsender/main.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

httpserver: src/httpserver/main.c $(SERVER_SRC) $(RESPONSE_SRC) $(REQUEST_SRC) $(HEADERS_SRC) $(SOCKET_READER_SRC) 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_request: tests/internal/request/test_request.c $(REQUEST_SRC) $(HEADERS_SRC)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LIBS)

test_headers: tests/internal/headers/test_headers.c $(HEADERS_SRC)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LIBS)

test: test_request test_headers
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
