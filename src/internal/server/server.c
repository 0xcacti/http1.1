#include <internal/server/server.h>
#include <libmill.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

server_t *serve(int port) {
    server_t *server = malloc(sizeof(*server));
    if (!server)
        return NULL;

    server->listener = tcplisten(iplocal("0.0.0.0", port, IPADDR_IPV4), 10);
    if (!server->listener) {
        free(server);
        return NULL;
    }

    server->closed = 0;
    server->done = chmake(int, 0);
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
    /* 1) drain until end of headers */
    char buf[1024];
    int n;
    size_t total = 0;
    while ((n = tcprecv(conn, buf, sizeof(buf), now() + 1000)) > 0) {
        total += n;
        if (total >= 4 && strstr(buf + (total - 4), "\r\n\r\n"))
            break;
    }

    /* 2) send response */
    static const char *response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Connection: close\r\n"
                                  "\r\n"
                                  "Hello World!\n";
    tcpsend(conn, response, strlen(response), -1);
    tcpflush(conn, -1);

    /* 3) graceful close */
    tcpshutdown(conn, 1);
    tcpclose(conn);
}
