CC = gcc
CFLAGS = -Wall -Wextra -fno-stack-check
LIBMILL_CFLAGS := $(shell pkg-config --cflags libmill 2>/dev/null || echo "-I/usr/local/include")
LIBMILL_LIBS := $(shell pkg-config --libs libmill 2>/dev/null || echo "-L/usr/local/lib -lmill")
CFLAGS += $(LIBMILL_CFLAGS)
LDFLAGS = $(LIBMILL_LIBS)

TEST_CFLAGS = $(CFLAGS)
TEST_LIBS = -lcriterion

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
