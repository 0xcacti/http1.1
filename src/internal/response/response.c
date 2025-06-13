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
