CC = gcc
CFLAGS = -Wall -Wextra 

all: http

http: src/main.c src/http.c
	$(CC) $(CFLAGS) -o $@ $^

clean: 
	rm -f http
