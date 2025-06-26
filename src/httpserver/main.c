#include "internal/response/response.h"
#include "internal/server/server.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>

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

void forward_proxy(response_writer_t *w, request_t *req, const char *target) {
    printf("do we hit this branch\n");
    ipaddr addr = ipremote("httpbin.org", 80, IPADDR_IPV4, -1);
    tcpsock sock = tcpconnect(addr, -1);
    if (sock == NULL) {
        handle_500(w, req);
        return;
    }

    char reqbuf[512];
    int req_len = snprintf(reqbuf, sizeof(reqbuf),
                           "GET %s HTTP/1.1\r\n"
                           "Host: httpbin.org\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           target);
    if (tcpsend(sock, reqbuf, req_len, -1) != (size_t)req_len || tcpflush(sock, -1) < 0) {
        tcpclose(sock);
        handle_500(w, req);
        return;
    }

    char buf[4096];
    size_t buf_len = 0;
    int headers_done = 0;

    while (1) {
        printf("we make it to here\n");
        ssize_t r = tcprecv(sock, buf + buf_len, sizeof(buf) - buf_len - 1, -1);
        printf("stuck on tcprecv\n");
        if (r < 0) {
            tcpclose(sock);
            handle_500(w, req);
            return;
        }
        if (r == 0) {
            break;
        }
        buf_len += (size_t)r;
        buf[buf_len] = '\0';
        if (!headers_done) {
            char *eoh = strstr(buf, "\r\n\r\n");
            if (eoh == NULL) {
                if (buf_len == sizeof(buf)) {
                    tcpclose(sock);
                    handle_500(w, req);
                    return;
                }
                continue;
            }
            write_status_line(w, RESPONSE_STATUS_OK);
            headers_t *headers = get_default_headers(0);
            headers_delete(headers, "Content-Length");
            headers_set(headers, "Transfer-Encoding", "chunked");
            printf("going to write headers\n");
            fflush(stdout);
            write_headers(w, headers);
            free_headers(headers);

            size_t header_len = (eoh + 4) - buf;
            size_t body_len = buf_len - header_len;
            if (body_len > 0) {
                write_chunked_body(w, buf + header_len, body_len);
            }
            memmove(buf, buf + header_len, body_len);
            buf_len = body_len;
            headers_done = 1;
        } else {
            write_chunked_body(w, buf, buf_len);
            buf_len = 0;
        }
    }
    write_chunked_body_done(w);
    tcpclose(sock);
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
    if (strncmp(method, "/httpbin", 8) == 0) {
        printf("we made it");
        const char *target = method + 8;
        forward_proxy(w, req, target);
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
