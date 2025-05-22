#include <errno.h>
#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 42069
#define BUFFER_SIZE 8

coroutine void dialogue(tcpsock as, chan lines_ch, chan done_ch) {
    char buffer[BUFFER_SIZE];
    char *current_line_contents = NULL;
    size_t line_capacity = 0;
    size_t line_len = 0;

    while (1) {
        size_t sz = tcprecv(as, buffer, sizeof(buffer), -1);
        if (sz == 0) {
            break; // Connection closed
        }

        // Process the received data
        char *start = buffer;
        for (size_t i = 0; i < sz; i++) {
            if (buffer[i] == '\n') {
                size_t part_len = &buffer[i] - start;
                size_t total_len = line_len + part_len;

                // Reallocate if needed
                if (total_len > line_capacity) {
                    size_t new_capacity = total_len + 64;
                    char *new_buffer = realloc(current_line_contents, new_capacity + 1);
                    if (!new_buffer) {
                        free(current_line_contents);
                        chs(done_ch, int, 1);
                        return;
                    }
                    current_line_contents = new_buffer;
                    line_capacity = new_capacity;
                }

                // Append and send line
                if (part_len > 0) {
                    memcpy(current_line_contents + line_len, start, part_len);
                }
                current_line_contents[total_len] = '\0';
                chs(lines_ch, char *, current_line_contents);

                // Reset for next line
                current_line_contents = NULL;
                line_capacity = line_len = 0;
                start = &buffer[i + 1];
            }
        }

        // Handle remaining data without newline
        size_t remaining = &buffer[sz] - start;
        if (remaining > 0) {
            size_t new_len = line_len + remaining;
            if (new_len > line_capacity) {
                size_t new_capacity = new_len + 64;
                char *new_buffer = realloc(current_line_contents, new_capacity + 1);
                if (!new_buffer) {
                    free(current_line_contents);
                    chs(done_ch, int, 1);
                    return;
                }
                current_line_contents = new_buffer;
                line_capacity = new_capacity;
            }
            memcpy(current_line_contents + line_len, start, remaining);
            line_len = new_len;
        }
    }

    // Send any remaining content
    if (line_len > 0 && current_line_contents) {
        current_line_contents[line_len] = '\0';
        chs(lines_ch, char *, current_line_contents);
    } else if (current_line_contents) {
        free(current_line_contents);
    }

    chdone(lines_ch, char *, NULL);
    chs(done_ch, int, 1);
}

int main(void) {
    // Disable output buffering
    setbuf(stdout, NULL);

    ipaddr addr = iplocal(NULL, PORT, 0);
    tcpsock ls = tcplisten(addr, 10);
    if (!ls) {
        perror("Can't open listening socket");
        return 1;
    }

    msleep(now() + 10);

    printf("Listening for TCP traffic on :%d\n", PORT);

    while (1) {
        tcpsock as = tcpaccept(ls, -1);
        if (!as) {
            continue;
        }

        // Get remote address for logging
        ipaddr remote_addr = tcpaddr(as);
        char addr_str[256];
        ipaddrstr(remote_addr, addr_str);
        printf("Accepted connection from %s\n", addr_str);

        // Create channels
        chan lines_ch = chmake(char *, 0);
        chan done_ch = chmake(int, 0);

        // Start coroutine to read lines
        go(dialogue(as, lines_ch, done_ch));

        // Read lines from channel
        while (1) {
            // Use choose to wait for either a line or completion
            choose {
                in(lines_ch, char *, line) : if (line) {
                    printf("%s\n", line);
                    free(line);
                }
                in(done_ch, int, done) : {
                    (void)done;
                    goto connection_closed;
                }
                end
            }
        }

    connection_closed:
        printf("Connection to  %s closed\n", addr_str);
        tcpclose(as);
        chclose(lines_ch);
        chclose(done_ch);
    }

    return 0;
}
