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
    if (server == NULL)
        return;

    tcpclose(server->listener);
    (void)chr(server->done, int);
    chclose(server->done);
    free(server);
}

coroutine void listen_routine(server_t *server) {
    while (1) {
        tcpsock conn = tcpaccept(server->listener, -1);
        if (!conn) {
            if (server->closed)
                break;
            printf("Error accepting connection\n");
            continue;
        }
        go(handle_connection(conn));
    }
    chs(server->done, int, 1);
}

coroutine void handle_connection(tcpsock conn) {
    char buf[1024];
    int n;
    size_t received = 0;
    while ((n = tcprecv(conn, buf, sizeof(buf), now() + 1000)) > 0) {
        received += n;
        if (received >= 4 && strstr(buf + (received > 4 ? received - 4 : 0), "\r\n\r\n"))
            break;
    }

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Hello World!\n";

    tcpsend(conn, response, strlen(response), -1);
    tcpflush(conn, -1);
    tcpshutdown(conn, 1);
    tcpclose(conn);
    return;
}
