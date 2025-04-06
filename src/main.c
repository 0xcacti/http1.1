#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test(void) {}

int main(void) {
    FILE *file;
    file = fopen("messages.txt", "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

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
        printf("Bytes read: %s\n", buf);
    }

    fclose(file);
    return 0;
}
