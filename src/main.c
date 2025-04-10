#include <errno.h>
#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAP 1024;
#define PORT 42069;
#define CONNECTION_BACKLOG 10;

coroutine void lines_reader(int socket, int ch) {
    char *currentLine = malloc(1024);
    if (!currentLine) {
        perror("malloc failed");
        tcp_close(socket, -1);
        chdone(ch);
        return;
    }
    currentLine[0] = '\0';
    size_t len = 0;

    for (;;) {
        char buf[9] = {0};
        ssize_t bytes_read = brecv(socket, buf, 8, -1);
        if (bytes_read < 0) {
            if (errno == ECONNRESET || errno == EPIPE) {
                break; // Connection closed
            }
            perror("tcp_recv");
            break;
        }
        if (bytes_read == 0) {
            break; // EOF, connection closed
        }

        buf[bytes_read] = '\0';
        char *newline = strchr(buf, '\n');
        if (newline != NULL) {
            *newline = '\0';
            size_t part1Len = strlen(buf);
            size_t newLen = len + part1Len;
            if (newLen + 1 > capacity) {
                capacity = newLen + 1;
                char *tmp = realloc(currentLine, capacity);
                if (!tmp) {
                    perror("realloc failed");
                    free(currentLine);
                    fclose(file);
                    chdone(ch);
                    return;
                }
                currentLine = tmp;
            }
            strcat(currentLine, buf); // Append part before newline

            // First send the length of the string
            size_t messageLen = strlen(currentLine) + 1; // Include null terminator
            int rc = chsend(ch, &messageLen, sizeof(messageLen), -1);
            if (rc < 0) {
                perror("chsend size");
                free(currentLine);
                fclose(file);
                chdone(ch);
                return;
            }

            // Then send the actual string
            rc = chsend(ch, currentLine, messageLen, -1);
            if (rc < 0) {
                perror("chsend message");
                free(currentLine);
                fclose(file);
                chdone(ch);
                return;
            }

            // Reset and copy part after newline
            char *part2 = newline + 1;
            len = strlen(part2);
            if (len + 1 > capacity) {
                capacity = len + 1;
                char *tmp = realloc(currentLine, capacity);
                if (!tmp) {
                    perror("realloc failed");
                    free(currentLine);
                    fclose(file);
                    chdone(ch);
                    return;
                }
                currentLine = tmp;
            }
            strcpy(currentLine, part2); // Overwrite, not append
            len = strlen(currentLine);  // Update len after overwrite
        } else {
            size_t bufLen = strlen(buf);
            size_t newLen = len + bufLen;
            if (newLen + 1 > capacity) {
                capacity = newLen + 1;
                char *tmp = realloc(currentLine, capacity);
                if (!tmp) {
                    perror("realloc failed");
                    free(currentLine);
                    fclose(file);
                    chdone(ch);
                    return;
                }
                currentLine = tmp;
            }
            strcat(currentLine, buf);
            len = newLen;
        }
    }

    // If there's a partial line at the end, send it
    if (strlen(currentLine) > 0) {
        size_t messageLen = strlen(currentLine) + 1;
        int rc = chsend(ch, &messageLen, sizeof(messageLen), -1);
        if (rc >= 0) {
            chsend(ch, currentLine, messageLen, -1);
        }
    }

    free(currentLine);
    fclose(file);
    chdone(ch);
}

int main(void) {
    struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, 42069, 0);
    if (rc < 0) {
        perror("ipaddr_local");
        return 1;
    }

    int listen_socket = tcp_listen(&addr, 10);
    if (listen_socket < 0) {
        perror("tcp_listen");
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct ipaddr client_addr;
        int client_socket = tcp_accept(listen_socket, NULL, -1);
        if (client_socket < 0) {
            perror("tcp_accept");
            continue;
        }

        char addr_str[IPADDR_MAXSTRLEN];
        ipaddr_str(&client_addr, addr_str);
        printf("Accepted connection from %s\n", addr_str);

        int cr = go(lines_reader(client_socket));

        result = chrecv(ch[0], line, msg_size, -1);
        if (result < 0) {
            if (errno == EPIPE) {
                free(line);
                break;
            }
            perror("chrecv message");
            free(line);
            break;
        }
        printf("read: %s\n", line);
        free(line);
    }

    hclose(ch[0]);
    return 0;
}
