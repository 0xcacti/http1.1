#include <internal/headers/headers.h>
#include <internal/response/response.h>
#include <libmill.h>
#include <stdio.h>

void response_writer_init(response_writer_t *writer, tcpsock conn) {
    if (writer) {
        writer->conn = conn;
        writer->state = STATUS_LINE;
    }
}

int write_status_line(response_writer_t *w, response_status_t status) {
    const char *reason_phrase;
    switch (status) {
    case RESPONSE_STATUS_OK:
        reason_phrase = "OK";
        break;
    case RESPONSE_STATUS_BAD_REQUEST:
        reason_phrase = "Bad Request";
        break;
    case RESPONSE_STATUS_NOT_FOUND:
        reason_phrase = "Not Found";
        break;
    case RESPONSE_STATUS_INTERNAL_ERROR:
        reason_phrase = "Internal Server Error";
        break;
    default:
        return -1; // Invalid status
    }

    char status_line[256];
    int len =
        snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", status, reason_phrase);
    if (len < 0 || len >= (int)sizeof(status_line)) {
        return -1;
    }

    w->state = HEADERS;
    return tcpsend(w->conn, status_line, len, -1);
}

headers_t *get_default_headers(int content_len) {
    headers_t *headers = new_headers();
    if (!headers) {
        return NULL;
    }

    char content_len_str[32];
    snprintf(content_len_str, sizeof(content_len_str), "%d", content_len);

    if (headers_set(headers, "Content-Length", content_len_str) != 0 ||
        headers_set(headers, "Content-Type", "text/plain") != 0 ||
        headers_set(headers, "Connection", "close") != 0) {
        free_headers(headers);
        return NULL;
    }

    return headers;
}

int write_headers(response_writer_t *w, headers_t *headers) {
    if (!w->conn || !headers || !headers->map) {
        return -1;
    }

    char header_line[1024];
    khint_t k;

    for (k = 0; k < kh_end(headers->map); ++k) {
        if (kh_exist(headers->map, k)) {
            const char *key = kh_key(headers->map, k);
            const char *value = kh_value(headers->map, k);

            int len = snprintf(header_line, sizeof(header_line), "%s: %s\r\n", key, value);
            if (len < 0 || len >= (int)sizeof(header_line)) {
                return -1;
            }

            if (tcpsend(w->conn, header_line, len, -1) < 0) {
                return -1;
            }
        }
    }

    const char *te = headers_get(headers, "Transfer-Encoding");
    if (te && strcmp(te, "chunked") == 0) {
        w->state = CHUNKED_BODY;
    } else {
        w->state = BODY;
    }
    return tcpsend(w->conn, "\r\n", 2, -1);
}

ssize_t write_body(response_writer_t *w, const char *buf, size_t len) {
    if (w->state != BODY) {
        return -1;
    }

    w->state = STATUS_LINE;
    int r = tcpsend(w->conn, buf, len, -1);
    if (r < 0) {
        return -1;
    }
    return r;
}

int write_chunked_body(response_writer_t *writer, const char *data, size_t length) {
    if (writer->state != CHUNKED_BODY) {
        return -1;
    }

    size_t buf_size = sizeof(size_t) * 2 + 1;
    char *size_hex = malloc(buf_size);
    if (size_hex == NULL) {
        return -1;
    }
    snprintf(size_hex, buf_size, "%zx\r\n", length);
    int r = tcpsend(writer->conn, size_hex, strlen(size_hex), -1);
    free(size_hex);
    if (r < 0) return -1;
    r = tcpsend(writer->conn, data, length, -1);
    if (r < 0) return -1;
    int r_e = tcpsend(writer->conn, "\r\n", 2, -1);
    if (r_e < 0) return -1;
    return r;
}

int write_chunked_body_done(response_writer_t *writer) {
    int r = tcpsend(writer->conn, "0\r\n", 3, -1);
    if (r < 0) {
        return -1;
    }
    writer->state = TRAILERS;
    return r;
}

int write_trailers(response_writer_t *writer, headers_t *trailers) {
    if (writer->state != TRAILERS || !trailers || !trailers->map) {
        return -1;
    }

    char trailer_line[1024];
    khint_t k;

    for (k = 0; k < kh_end(trailers->map); ++k) {
        if (kh_exist(trailers->map, k)) {
            const char *key = kh_key(trailers->map, k);
            const char *value = kh_value(trailers->map, k);

            int len = snprintf(trailer_line, sizeof(trailer_line), "%s: %s\r\n", key, value);
            if (len < 0 || len >= (int)sizeof(trailer_line)) {
                return -1;
            }

            if (tcpsend(writer->conn, trailer_line, len, -1) < 0) {
                return -1;
            }
        }
    }

    return tcpsend(writer->conn, "\r\n", 2, -1);
}
