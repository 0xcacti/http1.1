#ifndef INTERNAL_REQUEST_REQUEST_H
#define INTERNAL_REQUEST_REQUEST_H

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum parser_state {
    INITIALIZED,
    DONE,
} parser_state_t;

typedef struct request_line {
    char *http_version;
    char *request_target;
    char *method;
} request_line_t;

typedef struct request {
    parser_state_t state;
    request_line_t *line;
} request_t;

typedef ssize_t (*reader_func_t)(void *context, char *buffer, size_t max_bytes);

int is_alphabetic_uppercase(const char *str);
int request_from_reader(reader_func_t reader, void *read_context, request_t *out_request);
void free_request(request_t *request);


#endif // INTERNAL_REQUEST_REQUEST_H
