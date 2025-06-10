#include "khash.h"
#include <ctype.h>
#include <internal/headers/headers.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

const char *CLRF = "\r\n";

int find_bytes(const char *data, size_t len, const char *pattern, size_t pattern_len) {
    if (pattern_len > len)
        return -1;

    for (size_t i = 0; i <= len - pattern_len; i++) {
        if (strncmp(data + i, pattern, pattern_len) == 0) {
            return (int)i;
        }
    }

    return -1;
}

char *trim_whitespace(const char *str, size_t len) {
    size_t start = 0;
    while (start < len && isspace(str[start])) {
        start++;
    }

    size_t end = len;
    while (end > start && isspace(str[end - 1])) {
        end--;
    }

    size_t new_len = end - start;

    char *result = malloc(new_len + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, str + start, new_len);
    result[new_len] = '\0';

    return result;
}

char to_lower_char(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int to_lower(char *str) {
    if (str == NULL) {
        return -1; // Error: null pointer
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        str[i] = to_lower_char(str[i]);
    }
    return 0; // Success
}

bool is_invalid(unsigned char b) {

    if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9')) {
        return false;
    }

    const char *allowed_special = "!#$%&'*+-.^_`|~";
    if (strchr(allowed_special, b) != NULL) {
        return false;
    }
    return true;
}

headers_t *new_headers() {
    headers_t *headers = malloc(sizeof(headers_t));
    if (headers == NULL) {
        return NULL;
    }

    headers->map = kh_init(headers);
    if (headers->map == NULL) {
        free(headers);
        return NULL;
    }

    return headers;
}

void free_headers(headers_t *headers) {
    if (headers == NULL) {
        return;
    }

    khint_t k;
    for (k = 0; k < kh_end(headers->map); ++k) {
        if (kh_exist(headers->map, k)) {
            /* free both the key and the value */
            free((char *)kh_key(headers->map, k));
            free((char *)kh_value(headers->map, k));
        }
    }
    kh_destroy(headers, headers->map);
    free(headers);
}

parse_result_t parse_headers(headers_t *h, const char *data, size_t len) {
    parse_result_t result = {0, false, NULL};
    int idx = find_bytes(data, len, CLRF, 2);
    if (idx == -1)
        return result;
    if (idx == 0) {
        result.n = 2;
        result.done = true;
        return result;
    }

    const char *header_line = data;
    size_t header_line_len = idx;
    const char *colon_pos = memchr(header_line, ':', header_line_len);
    if (colon_pos == NULL) {
        result.error = strdup("invalid header: no colon found");
        return result;
    }

    int colon_idx = colon_pos - header_line;
    if (colon_idx > 0 && isspace(header_line[colon_idx - 1])) {
        result.error = strdup("invalid header field name: whitespace before colon");
        return result;
    }

    char *field_name = malloc(colon_idx + 1);
    if (field_name == NULL) {
        result.error = strdup("memory allocation failed");
        return result;
    }
    memcpy(field_name, header_line, colon_idx);
    field_name[colon_idx] = '\0';
    for (int i = 0; i < colon_idx; i++) {
        if (is_invalid((unsigned char)field_name[i])) {
            free(field_name);
            result.error = strdup("invalid header field name");
            return result;
        }
    }

    const char *field_value_start = header_line + colon_idx + 1;
    size_t field_value_len = header_line_len - colon_idx - 1;
    char *field_value = trim_whitespace(field_value_start, field_value_len);
    if (field_value == NULL) {
        free(field_name);
        result.error = strdup("memory allocation failed");
        return result;
    }

    int ret;
    int to_lower_result = to_lower(field_name);
    if (to_lower_result != 0) {
        free(field_name);
        free(field_value);
        result.error = strdup("failed to convert header field name to lowercase");
        return result;
    }

    khint_t k = kh_put(headers, h->map, field_name, &ret);
    if (ret == -1) {
        free(field_name);
        free(field_value);
        result.error = strdup("failed to add header to map");
        return result;
    }
    if (ret == 0) {
        free(field_name);
        free((char *)kh_value(h->map, k));
    }
    kh_value(h->map, k) = field_value;

    result.n = idx + 2;
    result.done = false;
    return result;
}
