#ifndef INTERNAL_SERVER_SERVER_H
#define INTERNAL_SERVER_SERVER_H

#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    tcpsock listener;
    volatile int closed;
} server_t;

coroutine void listen_routine(server_t *server);
coroutine void handle_connection(tcpsock conn);

server_t *serve(int port);
int close_server(server_t*);

#endif
