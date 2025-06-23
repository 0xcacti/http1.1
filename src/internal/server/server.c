#include <internal/response/response.h>
#include <internal/server/server.h>
#inc
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

    /* 1) tell the listener loop to break */
    server->closed = 1;

    /* 2) block until it acks (via chs) that it’s fully out of tcpaccept() */
    (void)chr(server->done, int);

    /* 3) now safe to close the FD and free resources */
    tcpclose(server->listener);
    chclose(server->done);
    free(server);
}

coroutine void listen_routine(server_t *server) {
    const int64_t timeout_ms = 50;
    while (!server->closed) {
        tcpsock conn = tcpaccept(server->listener, now() + timeout_ms);
        if (conn) {
            go(handle_connection(conn));
        } else {
            int err = errno;
            if (err == ETIMEDOUT) {
                /* normal – just no new connection yet */
                continue;
            }
            if (!server->closed) {
                /* real error */
                fprintf(stderr, "Error accepting connection: %s\n", strerror(err));
            }
        }
    }
    /* signal to close_server() that we’re done */
    chs(server->done, int, 1);
}
coroutine void handle_connection(tcpsock conn) {
    request_t *req = malloc(sizeof(request_t));
    if (req == NULL) {
        fprintf(stderr, "Error allocating request\n");
        tcpshutdown(conn, 1);
        tcpclose(conn);
        return;
    }

    socket_context_t ctx = {.socket = conn};
    request_t request;
    int success = request_from_reader(conn, &ctx, req);

    // const char *body = "Hello World!";
    // int body_length = strlen(body);

    write_status_line(conn, RESPONSE_STATUS_OK);
    headers_t *headers = get_default_headers(0);
    write_headers(conn, headers);
    tcpflush(conn, -1);
    tcpshutdown(conn, 1);
    tcpclose(conn);
}
