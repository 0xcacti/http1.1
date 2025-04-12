#include <errno.h>
#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 42069
#define BUFFER_SIZE 1024
#define CONNECTION_BACKLOG 10

coroutine void lines_reader(int socket, int ch) {
    printf("DEBUG: lines_reader started with socket %d\n", socket);

    // Take ownership of the socket
    socket = hown(socket);
    if (socket < 0) {
        perror("hown failed");
        chdone(ch);
        return;
    }

    // Try a very small initial read with minimal timeout
    char byte;
    int64_t immediate_deadline = now() + 10; // Just 10ms
    int rc = brecv(socket, &byte, 1, immediate_deadline);

    if (rc == 0) {
        printf("DEBUG: Connection closed immediately\n");
    } else if (rc < 0) {
        if (errno == ETIMEDOUT) {
            printf("DEBUG: No data available immediately\n");
        } else {
            printf("DEBUG: Initial read failed: %s\n", strerror(errno));
        }
    } else {
        // Successfully got 1 byte!
        printf("DEBUG: Received initial byte: '%c' (0x%02x)\n", byte, (unsigned char)byte);

        // Try to get more data
        char buffer[1024] = {0};
        buffer[0] = byte;

        // Read as much as possible with a short timeout
        int64_t quick_deadline = now() + 50; // 50ms
        rc = brecv(socket, buffer + 1, sizeof(buffer) - 2, quick_deadline);

        if (rc == 0) {
            printf("DEBUG: Connection closed after initial byte\n");
        } else if (rc < 0 && errno == ETIMEDOUT) {
            printf("DEBUG: Got just the initial byte\n");
        } else if (rc < 0) {
            printf("DEBUG: Follow-up read failed: %s\n", strerror(errno));
        } else {
            // Successfully got more data!
            buffer[rc + 1] = '\0';
            printf("DEBUG: Received complete message: '%s'\n", buffer);

            // Send the message through the channel
            size_t msg_len = strlen(buffer) + 1;
            rc = chsend(ch, &msg_len, sizeof(msg_len), -1);
            if (rc >= 0) {
                chsend(ch, buffer, msg_len, -1);
            }
        }
    }

    printf("DEBUG: lines_reader exiting\n");
    tcp_close(socket, -1);
    chdone(ch);
}

int main(void) {
    struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, PORT, 0);
    if (rc < 0) {
        perror("ipaddr_local");
        return 1;
    }

    int listen_socket = tcp_listen(&addr, CONNECTION_BACKLOG);
    if (listen_socket < 0) {
        perror("tcp_listen");
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct ipaddr client_addr;
        int client_socket = tcp_accept(listen_socket, &client_addr, -1);
        if (client_socket < 0) {
            perror("tcp_accept");
            continue;
        }

        char addr_str[IPADDR_MAXSTRLEN];
        ipaddr_str(&client_addr, addr_str);
        printf("Accepted connection from %s\n", addr_str);

        int ch[2];
        if (chmake(ch) != 0) {
            perror("chmake");
            tcp_close(client_socket, -1);
            continue;
        }

        int cr = go(lines_reader(client_socket, ch[1]));
        if (cr < 0) {
            perror("go");
            tcp_close(client_socket, -1);
            hclose(ch[0]);
            hclose(ch[1]);
            continue;
        }

        printf("DEBUG: Main loop waiting for messages\n");

        while (1) {
            printf("DEBUG: About to receive message size\n");

            size_t msg_size;
            ssize_t result = chrecv(ch[0], &msg_size, sizeof(msg_size), -1);

            printf("DEBUG: chrecv for size returned %zd, msg_size=%zu, errno=%d (%s)\n", result,
                   msg_size, errno, strerror(errno));

            if (result < 0) {
                if (errno == EPIPE) {
                    printf("DEBUG: Channel closed\n");
                    break;
                }
                perror("chrecv size");
                break;
            }

            printf("DEBUG: Allocating buffer for message of size %zu\n", msg_size);
            char *line = malloc(msg_size);
            if (!line) {
                perror("malloc");
                break;
            }

            printf("DEBUG: About to receive message content\n");
            result = chrecv(ch[0], line, msg_size, -1);

            printf("DEBUG: chrecv for content returned %zd, errno=%d (%s)\n", result, errno,
                   strerror(errno));

            if (result < 0) {
                if (errno == EPIPE) {
                    printf("DEBUG: Channel closed while receiving content\n");
                    free(line);
                    break;
                }
                perror("chrecv message");
                free(line);
                break;
            }

            printf("RECEIVED LINE: %s\n", line);
            free(line);
        }

        printf("Connection to %s closed\n", addr_str);
        hclose(ch[0]);
        hclose(ch[1]);
    }

    tcp_close(listen_socket, -1);
    return 0;
}
