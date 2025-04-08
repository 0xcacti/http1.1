#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>

coroutine void worker(const char *text) {
    while (1) {
        printf("%s\n", text);
        msleep(now() + random() % 500);
    }
}

int main() {
    // This is the correct way to use go() in libdill
    int h = go(worker("Hello!"));
    int w = go(worker("World!"));

    // Wait for 5 seconds
    msleep(now() + 5000);
    return 0;
}
