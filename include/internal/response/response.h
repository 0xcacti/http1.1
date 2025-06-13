#ifndef INTERNAL_RESPONSE_RESPONSE_H
#define INTERNAL_RESPONSE_RESPONSE_H

#include <libmill.h>
#include <internal/headers/headers.h>

typedef enum {
    RESPONSE_STATUS_OK = 200,
    RESPONSE_STATUS_BAD_REQUEST = 400,
    RESPONSE_STATUS_NOT_FOUND = 404,
    RESPONSE_STATUS_INTERNAL_ERROR = 500,
} response_status_t;

int write_status_line(tcpsock conn, response_status_t status);
headers_t *get_default_headers(int content_len);
int write_headers(tcpsock conn, headers_t *headers);

#endif
