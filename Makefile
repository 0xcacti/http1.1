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

all: tcplistener udpsender

tcplistener: src/tcplistener/main.c src/tcplistener/http.c $(REQUEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

udpsender: src/udpsender/main.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_request: tests/internal/request/test_request.c $(REQUEST_SRC) 
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LIBS)

test: test_request
	./test_request

setup: 
	mkdir -p include/internal/request
	mkdir -p src/internal/request
	mkdir -p tests/internal/request

clean:
	rm -f tcplistener udpsender test_request

.PHONY: all test setup clean
