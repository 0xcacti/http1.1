#include <internal/server/server.h>
#include <libmill.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

server_t *serve(int port) {
    server_t *server = malloc(sizeof(server_t));
    if (!server)
        return NULL;

    server->closed = 0;
    server->listener = tcplisten(iplocal("0.0.0.0", port, IPADDR_IPV4), 10);
    if (!server->listener) {
        free(server);
        return NULL;
    }
    server->done = chmake(int, 0);

    go(listen_routine(server));

    return server;
}
void close_server(server_t *server) {
    if (server == NULL)
        return;

    server->closed = 1;
    if (server->listener != NULL) {
        tcpclose(server->listener);
    }
    (void)chr(server->done, int);
    chclose(server->done);
    free(server);
}

coroutine void listen_routine(server_t *server) {
    while (!server->closed) {
        tcpsock conn = tcpaccept(server->listener, -1);

        if (!conn) {
            if (server->closed) {
                break;
            }
            printf("Error accepting connection\n");
            continue;
        }

        go(handle_connection(conn));
    }
    chs(server->done, int, -1);
}

coroutine void handle_connection(tcpsock conn) {
    // defer conn.Close() equivalent - close on function exit

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Hello World!\n";

    // conn.Write([]byte(response)) equivalent
    tcpsend(conn, response, strlen(response), -1);
    tcpflush(conn, -1);

    // Close connection (defer equivalent)
    tcpshutdown(conn, 1);
    return;
}
