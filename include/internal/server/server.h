#ifndef INTERNAL_SERVER_SERVER_H
#define INTERNAL_SERVER_SERVER_H

#include <libmill.h>
#include <internal/request/request.h>
#include <internal/response/response.h>

typedef void handler(response_writer_t *w, request_t *req);

typedef struct {
    tcpsock listener;
    int closed;
    chan done;
    handler *handler;
} server_t;

coroutine void listen_routine(server_t *server);
coroutine void handle_connection(tcpsock conn);

server_t *serve(int port, handler *handler);
void close_server(server_t*);

#endif
