#include <errno.h>
#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

coroutine void lines_reader(FILE *file, int ch) {
    char *currentLine = malloc(1024);
    if (!currentLine) {
        perror("malloc failed");
        fclose(file);
        chdone(ch);
        return;
    }
    currentLine[0] = '\0';
    size_t capacity = 1024;
    size_t len = 0;

    for (;;) {
        char buf[9] = {0};
        size_t bytesRead = fread(buf, 1, 8, file);
        if (bytesRead == 0) {
            if (!feof(file)) {
                perror("Error reading file");
            }
            break;
        }

        buf[bytesRead] = '\0';
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
            int rc = chsend(ch, currentLine, newLen + 1, -1);
            if (rc < 0) {
                perror("chsend");
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

    free(currentLine);
    fclose(file);
    chdone(ch);
}

int main(void) {
    FILE *file = fopen("messages.txt", "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    int ch[2];
    if (chmake(ch) != 0) {
        perror("chmake");
        fclose(file);
        return 1;
    }

    int cr = go(lines_reader(file, ch[1]));
    if (cr < 0) {
        perror("Error creating coroutine");
        chdone(ch[1]);
        hclose(ch[0]);
        hclose(ch[1]);
        fclose(file);
        return 1;
    }

    while (1) {
        size_t msg_size;
        ssize_t result = chrecv(ch[0], &msg_size, sizeof(msg_size), -1);
        if (result < 0) {
            if (errno == EPIPE)
                break;
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
