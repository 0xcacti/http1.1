#include "internal/server/server.h"
#include <signal.h>

static volatile int shutdown_requested = 0;

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    shutdown_requested = 1;
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server_t *server = serve(42069);
    if (!server) {
        printf("Error starting server\n");
        return 1;
    }

    printf("Server started on port %d\n", 42069);

    while (!shutdown_requested) {
        msleep(now() + 100);
    }

    close_server(server);
    return 0;
}
