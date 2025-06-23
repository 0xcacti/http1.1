#include "internal/response/response.h"
#include "internal/server/server.h"
#include <signal.h>
#include <stdio.h>

static volatile int shutdown_requested = 0;

void handle_400(response_writer_t *w, request_t *req) {
    (void)req;
    write_status_line(w, RESPONSE_STATUS_BAD_REQUEST);
    char *body = "<html>"
                 "<head>"
                 "<title>400 Bad Request</title>"
                 "</head>"
                 "<body>"
                 "<h1>Bad Request</h1>"
                 "<p>Your request honestly kinda sucked.</p>"
                 "</body>"
                 "</html>";
    int len_body = strlen(body);
    headers_t *headers = get_default_headers(len_body);
    if (headers == NULL) {
        fprintf(stderr, "Error creating headers\n");
        return;
    }
    headers_set(headers, "Content-Type", "text/html");
    write_headers(w, headers);
    write_body(w, body, len_body);
}

void handle_500(response_writer_t *w, request_t *req) {
    (void)req;
    write_status_line(w, RESPONSE_STATUS_INTERNAL_ERROR);
    char *body = "<html>"
                 "<head>"
                 "<title>500 Internal Server Error</title>"
                 "</head>"
                 "<body>"
                 "<h1>Internal Server Error</h1>"
                 "<p>Okay, you know what? This one is on me.</p>"
                 "</body>"
                 "</html>";
    int len_body = strlen(body);
    headers_t *headers = get_default_headers(len_body);
    if (headers == NULL) {
        fprintf(stderr, "Error creating headers\n");
        return;
    }
    headers_set(headers, "Content-Type", "text/html");
    write_headers(w, headers);
    write_body(w, body, len_body);
}

void handle_200(response_writer_t *w, request_t *req) {
    (void)req;
    write_status_line(w, RESPONSE_STATUS_OK);
    char *body = "<html>"
                 "<head>"
                 "<title>200 OK</title>"
                 "</head>"
                 "<body>"
                 "<h1>Success!</h1>"
                 "<p>Your request was an absolute banger.</p>"
                 "</body>"
                 "</html>";
    int len_body = strlen(body);
    headers_t *headers = get_default_headers(len_body);
    if (headers == NULL) {
        fprintf(stderr, "Error creating headers\n");
        return;
    }
    headers_set(headers, "Content-Type", "text/html");
    write_headers(w, headers);
    write_body(w, body, len_body);
}

void handle(response_writer_t *w, request_t *req) {
    char *method = req->request_line->request_target;
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

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    shutdown_requested = 1;
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server_t *server = serve(42069, handle);
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
