#include <stdio.h>

int main(void) {
    FILE *filePtr;
    filePtr = fopen("messages.txt", "r");
    if (filePtr == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    fclose(filePtr);
    return 0;
}
