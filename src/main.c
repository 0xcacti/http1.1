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
    int retries = 0;

    while (1) {
        errno = 0; // Clear errno before each read
        size_t sz = tcprecv(as, buffer, sizeof(buffer), -1);
        int saved_errno = errno;

        // If we got data, process it
        if (sz > 0) {
            retries = 0; // Reset retry counter on successful read

            // Find newlines in the buffer
            char *start = buffer;
            for (size_t i = 0; i < sz; i++) {
                if (buffer[i] == '\n') {
                    // Found newline - process the portion before it
                    size_t part_len = &buffer[i] - start;

                    if (part_len > 0 || line_len > 0) {
                        // Need to append this part to current line and send
                        size_t total_len = line_len + part_len;

                        // Ensure we have enough space
                        if (total_len > line_capacity) {
                            size_t new_capacity = total_len + 64;
                            char *new_buffer = realloc(current_line_contents, new_capacity + 1);
                            if (!new_buffer) {
                                free(current_line_contents);
                                tcpclose(as);
                                chs(done_ch, int, 1);
                                return;
                            }
                            current_line_contents = new_buffer;
                            line_capacity = new_capacity;
                        }

                        // Append the part
                        if (part_len > 0) {
                            memcpy(current_line_contents + line_len, start, part_len);
                        }

                        // Send the line
                        current_line_contents[total_len] = '\0';
                        chs(lines_ch, char *, current_line_contents);

                        // Reset for next line
                        current_line_contents = NULL;
                        line_capacity = line_len = 0;
                    }

                    start = &buffer[i + 1]; // Start next part after newline
                }
            }

            // Handle remaining part (no newline found)
            size_t remaining = &buffer[sz] - start;
            if (remaining > 0) {
                // Need to append to current line
                size_t new_len = line_len + remaining;

                if (new_len > line_capacity) {
                    size_t new_capacity = new_len + 64;
                    char *new_buffer = realloc(current_line_contents, new_capacity + 1);
                    if (!new_buffer) {
                        free(current_line_contents);
                        tcpclose(as);
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

        // If we got an error, try a few more times with timeouts
        if (saved_errno != 0 && retries < 3) {
            retries++;
            errno = 0;
            sz = tcprecv(as, buffer, sizeof(buffer), now() + 100);
            if (sz > 0) {
                saved_errno = 0; // Clear error if we got data
                continue;        // Go back to process this data
            }
        }

        // If we still have an error after retries, exit
        if (saved_errno != 0 && (sz == 0 || retries >= 3)) {
            // Send any remaining content
            if (line_len > 0 && current_line_contents) {
                current_line_contents[line_len] = '\0';
                chs(lines_ch, char *, current_line_contents);
                current_line_contents = NULL;
            }
            break;
        }
    }

    // Only free if we still own it
    if (current_line_contents) {
        free(current_line_contents);
    }
    chdone(lines_ch, char *, NULL);
    tcpclose(as);
    chs(done_ch, int, 1); // Signal that we're done
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
                in(done_ch, int, done) : goto connection_closed;
                end
            }
        }

    connection_closed:
        printf("Connection to  %s closed\n", addr_str);
        chclose(lines_ch);
        chclose(done_ch);
    }

    return 0;
}
