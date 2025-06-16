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

typedef enum {
    STATUS_LINE,
    HEADERS,
    BODY,
} writer_state_t;

typedef struct {
    tcpsock conn;
    writer_state_t state;
} response_writer_t;

void response_writer_init(response_writer_t *writer, tcpsock conn);
int response_writer_write_status_line(response_writer_t *writer, response_status_t status);
int response_writer_write_headers(response_writer_t *writer, headers_t *headers);
ssize_t response_writer_write_body(response_writer_t *writer, const char *data, size_t length);

#endif
