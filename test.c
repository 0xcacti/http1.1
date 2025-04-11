#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    struct ipaddr addr;
    ipaddr_local(&addr, NULL, 42069, 0);
    int listen_socket = tcp_listen(&addr, 10);
    if (listen_socket < 0) {
        perror("tcp_listen");
        return 1;
    }
    printf("Server listening on port 42069\n");

    int client_socket = tcp_accept(listen_socket, NULL, -1);
    if (client_socket < 0) {
        perror("tcp_accept");
        tcp_close(listen_socket, -1);
        return 1;
    }
    printf("Accepted connection from 127.0.0.1\n");

    char buf[100] = {0};
    ssize_t n = brecv(client_socket, buf, sizeof(buf) - 1, -1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Read: %s\n", buf);
    } else if (n == 0) {
        printf("Connection closed without data\n");
    } else {
        perror("brecv");
    }

    tcp_close(client_socket, -1);
    tcp_close(listen_socket, -1);
    return 0;
}
