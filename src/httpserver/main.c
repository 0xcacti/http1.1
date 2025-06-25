#include "internal/response/response.h"
#include "internal/server/server.h"
#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>

typedef struct {
    response_writer_t *w;
    request_t *req;
    CURL *curl;
    int called;
} proxy_ctx_t;

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

static size_t header_cb(char *buf, size_t size, size_t nmemb, void *userdata) {
    proxy_ctx_t *ctx = userdata;
    size_t len = size * nmemb;

    if (!ctx->called) {
        long status = 0;
        curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &status);

        if (write_status_line(ctx->w, (int)status) < 0) {
            handle_500(ctx->w, ctx->req);
            return 0;
        }
    }
    headers_t *hdrs = get_default_headers(0);
    headers_delete(hdrs, "Content-Length");
    headers_set(hdrs, "Transfer-Encoding", "chunked");
    if (write_headers(ctx->w, hdrs) < 0) {
        handle_500(ctx->w, ctx->req);
        free_headers(hdrs);
        return 0;
    }
    free_headers(hdrs);
    ctx->called = 1;

    return len;
}

void forward_proxy(response_writer_t *w, request_t *req, const char *target) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        handle_500(w, req);
        return;
    }

    char *url = malloc(strlen("https://httpbin.org") + strlen(target) + 1);
    sprintf(url, "https://httpbin.org%s", target);

    proxy_ctx_t ctx = {.w = w, .req = req, .called = 0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    if (curl_easy_perform(curl) != CURLE_OK) {
        handle_500(w, req);
    } else {
        if (write_chunked_body_done(w) < 0) {
            handle_500(w, req);
        }
    }
    free(url);
    curl_easy_cleanup(curl);
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
