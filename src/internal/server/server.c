#include <internal/server/server.h>
#include <libmill.h>
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

    go(listen_routine(server));

    return server;
}

void server_close(server_t *server) {
    if (server == NULL)
        return;

    server->closed = 1;
    if (server->listener != NULL) {
        tcpclose(server->listener);
    }
    free(server);
}

coroutine void listen_routine(server_t *server) {
    while (1) {
        tcpsock conn = tcpaccept(server->listener, -1);
        if (conn == NULL) {
            if (server->closed) {
                return;
            }
            printf("Error accepting connection\n");
            continue;
        }

        go(handle_connection(conn));
    }
}

coroutine void handle_connection(tcpsock conn) {
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Hello World!\n";
    tcpsend(conn, response, strlen(response), -1);
    tcpclose(conn);
}
