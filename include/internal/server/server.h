#ifndef INTERNAL_SERVER_SERVER_H
#define INTERNAL_SERVER_SERVER_H

#include <libmill.h>
#include <internal/request/request.h>

typedef struct {
    tcpsock listener;
    int closed;
    chan done;
} server_t;

void handler(request_t *req);

coroutine void listen_routine(server_t *server);
coroutine void handle_connection(tcpsock conn);

server_t *serve(int port);
void close_server(server_t*);

#endif
