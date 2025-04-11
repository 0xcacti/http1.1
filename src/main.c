#include <errno.h>
#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 42069
#define BUFFER_SIZE 1024
#define CONNECTION_BACKLOG 10

coroutine void lines_reader(int socket, int ch) {
    char *current_line = malloc(BUFFER_SIZE);
    if (!current_line) {
        perror("malloc failed");
        tcp_close(socket, -1);
        chdone(ch);
        return;
    }
    current_line[0] = '\0';

    for (;;) {
        char buf[9] = {0};
        ssize_t bytes_read = brecv(socket, buf, 8, -1);
        printf("Read %zd bytes: '%.*s'\n", bytes_read, (int)bytes_read,
               buf); // Debug: show what’s read
        if (bytes_read <= 0) {
            if (bytes_read < 0 && errno != ECONNRESET && errno != EPIPE) {
                perror("brecv");
            }
            if (strlen(current_line) > 0) {
                printf("Sending at EOF: '%s'\n", current_line); // Debug: show final send
                size_t msg_len = strlen(current_line) + 1;
                int rc = chsend(ch, &msg_len, sizeof(msg_len), -1);
                if (rc >= 0) {
                    chsend(ch, current_line, msg_len, -1);
                } else {
                    perror("chsend failed at EOF"); // Debug: catch send errors
                }
            }
            break;
        }

        buf[bytes_read] = '\0';
        char *buf_ptr = buf;
        char *newline;

        while ((newline = strchr(buf_ptr, '\n')) != NULL) {
            *newline = '\0';
            size_t part_len = newline - buf_ptr;
            size_t current_len = strlen(current_line);
            size_t total_len = current_len + part_len;
            char *combined = malloc(total_len + 1);
            if (!combined) {
                perror("malloc failed");
                free(current_line);
                tcp_close(socket, -1);
                chdone(ch);
                return;
            }
            strcpy(combined, current_line);
            strncat(combined, buf_ptr, part_len);

            printf("Sending line: '%s'\n", combined); // Debug: show each line sent
            size_t msg_len = total_len + 1;
            int rc = chsend(ch, &msg_len, sizeof(msg_len), -1);
            if (rc < 0) {
                perror("chsend size");
                free(combined);
                free(current_line);
                tcp_close(socket, -1);
                chdone(ch);
                return;
            }
            rc = chsend(ch, combined, msg_len, -1);
            if (rc < 0) {
                perror("chsend message");
                free(combined);
                free(current_line);
                tcp_close(socket, -1);
                chdone(ch);
                return;
            }

            free(combined);
            current_line[0] = '\0';
            buf_ptr = newline + 1;
        }

        size_t remaining_len = strlen(buf_ptr);
        size_t new_len = strlen(current_line) + remaining_len;
        if (new_len + 1 > BUFFER_SIZE) {
            fprintf(stderr, "Line too long, truncating\n");
            current_line[0] = '\0';
        }
        strncat(current_line, buf_ptr, BUFFER_SIZE - strlen(current_line) - 1);
        printf("Current line after append: '%s'\n", current_line); // Debug: show accumulation
    }

    free(current_line);
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

        // Create a channel (ch[0] for receiving and ch[1] for sending)
        int ch[2];
        if (chmake(ch) != 0) {
            perror("chmake");
            tcp_close(client_socket, -1);
            continue;
        }

        // Spawn the lines_reader coroutine using ch[1] as the sending endpoint.
        int cr = go(lines_reader(client_socket, ch[1]));
        if (cr < 0) {
            perror("go");
            tcp_close(client_socket, -1);
            hclose(ch[0]);
            hclose(ch[1]);
            continue;
        }

        // Receive and print lines from the channel (receive on ch[0])
        while (1) {
            size_t msg_size;
            ssize_t result = chrecv(ch[0], &msg_size, sizeof(msg_size), -1);
            if (result < 0) {
                if (errno == EPIPE)
                    break; // Channel closed
                perror("chrecv size");
                break;
            }

            char *line = malloc(msg_size);
            if (!line) {
                perror("malloc");
                break;
            }

            result = chrecv(ch[0], line, msg_size, -1);
            if (result < 0) {
                if (errno == EPIPE) {
                    free(line);
                    break; // Channel closed
                }
                perror("chrecv message");
                free(line);
                break;
            }

            printf("%s\n", line); // Print the received line
            free(line);
        }

        printf("Connection to %s closed\n", addr_str);
        hclose(ch[0]);
        hclose(ch[1]);
    }

    tcp_close(listen_socket, -1);
    return 0;
}
