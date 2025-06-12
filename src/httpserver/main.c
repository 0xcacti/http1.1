#include "internal/server/server.h"
#include <signal.h>

static volatile int shutdown_requested = 0;

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    shutdown_requested = 1;
}

int main(void) {
    const int PORT = 42069;
    server_t *server = serve(PORT);
    if (!server) {
        printf("Error starting server\n");
        return 1;
    }

    printf("Server started on port %d\n", PORT);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (!shutdown_requested) {
        msleep(now() + 100);
    }

    printf("Shutting down  server...\n");
    close_server(server);
    return 0;
}
