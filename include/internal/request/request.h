#ifndef INTERNAL_REQUEST_REQUEST_H
#define INTERNAL_REQUEST_REQUEST_H

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct request_line {
    char *http_version;
    char *request_target;
    char *method;
} request_line_t;

typedef struct request {
    request_line_t *line;
} request_t;

int is_alphabetic_uppercase(const char *str);
int request_from_header(const char *req, size_t length, request_t *out_request);
void free_request(request_t *request);


#endif // INTERNAL_REQUEST_REQUEST_H
