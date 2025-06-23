#include <internal/response/response.h>
#include <internal/server/server.h>
#include <internal/socket_reader/socket_reader.h>
#include <libmill.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

server_t *serve(int port, handler *handler) {
    server_t *server = malloc(sizeof(*server));
    if (!server)
        return NULL;
    if (handler == NULL) {
        fprintf(stderr, "Error: handler cannot be NULL\n");
        free(server);
        return NULL;
    }

    server->listener = tcplisten(iplocal("0.0.0.0", port, IPADDR_IPV4), 10);
    if (!server->listener) {
        free(server);
        return NULL;
    }

    server->closed = 0;
    server->done = chmake(int, 0);
    server->handler = handler;

    go(listen_routine(server));
    return server;
}

void close_server(server_t *server) {
    if (!server)
        return;

    server->closed = 1;
    (void)chr(server->done, int);
    tcpclose(server->listener);
    chclose(server->done);
    free(server);
}

coroutine void listen_routine(server_t *server) {
    const int64_t timeout_ms = 50;
    while (!server->closed) {
        tcpsock conn = tcpaccept(server->listener, now() + timeout_ms);
        response_writer_t *w = malloc(sizeof(response_writer_t));
        response_writer_init(w, conn);
        if (conn) {
            printf("we made it to the handle route moose\n");
            go(handle_connection(server, w));
        } else {
            int err = errno;
            if (err == ETIMEDOUT) {
                continue;
            }
            if (!server->closed) {
                fprintf(stderr, "Error accepting connection: %s\n", strerror(err));
            }
        }
    }
    chs(server->done, int, 1);
}

coroutine void handle_connection(server_t *s, response_writer_t *w) {
    request_t *req = malloc(sizeof(request_t));
    if (req == NULL) {
        fprintf(stderr, "Error allocating request\n");
        tcpshutdown(w->conn, 1);
        tcpclose(w->conn);
        free(w);
        return;
    }

    socket_context_t ctx = {.socket = w->conn};

    printf("we are trying to parse the request\n");
    int result = request_from_reader(socket_reader, &ctx, req);
    printf("we parsed the request\n");

    if (result < 0) {
        write_status_line(w, RESPONSE_STATUS_BAD_REQUEST);
        char *body = "Error parsing request";
        int body_len = strlen(body);
        headers_t *headers = get_default_headers(body_len);
        write_headers(w, headers);
        write_body(w, body, body_len);
        free(req);
        return;
    }

    printf("calling handler\n");
    s->handler(w, req);
    printf("handler called\n");
    tcpflush(w->conn, -1);
    tcpshutdown(w->conn, 1);
    tcpclose(w->conn);
    free_request(req);
    return;
}
