#ifndef INTERNAL_HEADERS_HEADERS_H
#define INTERNAL_HEADERS_HEADERS_H

#include "khash.h"
#include <stdbool.h>


KHASH_MAP_INIT_STR(headers, char*)

typedef struct {
    khash_t(headers) *map;
} headers_t;

typedef struct {
    int n;
    bool done;
    char *error;
} parse_result_t;

headers_t *new_headers(void);
void free_headers(headers_t *headers);
parse_result_t parse_headers(headers_t *h, const char *data, size_t len);
const char *headers_get(headers_t *h, const char *key);

#endif
