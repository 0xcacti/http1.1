#include "internal/server/server.h"
#include <signal.h>
#include <stdio.h>

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

void handle_400(response_writer_t *w, request_t *req) {
    (void)req;
}

void handle_500(response_writer_t *w, request_t *req) {}

void handle_200(response_writer_t *w, request_t *req) {}

void handle(response_writer_t *w, request_t *req) {
    char *method = req->request_line->method;
    if (strcmp(method, "/yourproblem") == 0) {
        handle_400(w, req);
        return;
    }
    if (strcmp(method, "/myproblem") == 0) {
        handle_500(w, req);
        return;
    }
    handle_200(w, req);
    return;
}
