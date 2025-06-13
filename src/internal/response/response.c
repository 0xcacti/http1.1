#include <internal/headers/headers.h>
#include <internal/response/response.h>
#include <libmill.h>
#include <stdio.h>

int write_status_line(tcpsock conn, response_status_t status) {
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

    return tcpsend(conn, status_line, len, -1);
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

int write_headers(tcpsock conn, headers_t *headers) {
    if (!conn || !headers || !headers->map) {
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

            if (tcpsend(conn, header_line, len, -1) < 0) {
                return -1;
            }
        }
    }

    return tcpsend(conn, "\r\n", 2, -1);
}
