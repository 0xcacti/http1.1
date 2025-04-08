#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *file;
    file = fopen("messages.txt", "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    char *currentLine = malloc(1024);
    currentLine[0] = '\0';

    for (;;) {
        char buf[9] = {0};
        size_t bytesRead = bytesRead = fread(buf, 1, 8, file);
        if (bytesRead == 0) {
            if (feof(file)) {
                break;
            } else {
                perror("Error reading file");
                fclose(file);
                return 1;
            }
        }

        char *newline = strchr(buf, '\n');
        if (newline != NULL) {
            *newline = '\0';
            strcat(currentLine, buf);
            printf("Bytes read: %s\n", currentLine);
            currentLine[0] = '\0';
            if (*(newline + 1)) {
                strcpy(currentLine, newline + 1);
            }
        } else {
            strcat(currentLine, buf);
        }
    }

    free(currentLine);
    fclose(file);
    return 0;
}
