#include <inttypes.h> // For PRId64 macro
#include <libmill.h>
#include <stdio.h>

int main() {
    printf("Starting bare minimum libmill test...\n");

    // Just get the current time - if libmill is working, this should succeed
    int64_t current = now();
    printf("Current time: %" PRId64 "\n", current);

    printf("Test completed successfully!\n");
    return 0;
}
