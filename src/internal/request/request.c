#include <ctype.h>
#include <internal/request/request.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_alphabetic_uppercase(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isupper(str[i]) || !isalpha(str[i])) {
            return 0;
        }
    }

    return 1;
}

int request_from_header(const char *req, size_t length, request_t *out_request) {
    if (req == NULL || out_request == NULL || length == 0) {
        return -1;
    }

    char *line, *saveptr;
    char *request_copy = strdup(req);
    if (request_copy == NULL) {
        return -1;
    }

    line = strtok_r(request_copy, "\r\n", &saveptr);
    if (line == NULL) {
        free(request_copy);
        return -1;
    }

    char *field_saveptr;
    char *method = strtok_r(line, " ", &field_saveptr);
    char *target = strtok_r(NULL, " ", &field_saveptr);
    char *version = strtok_r(NULL, " ", &field_saveptr);

    if (method == NULL || target == NULL || version == NULL) {
        free(request_copy);
        return -1;
    }

    if (!is_alphabetic_uppercase(method)) {
        free(request_copy);
        return -1;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        free(request_copy);
        return -1;
    }

    char *numeric_version = version + 5; // Skip "HTTP/"

    request_line_t *request_line = malloc(sizeof(request_line_t));
    if (request_line == NULL) {
        free(request_copy);
        return -1;
    }

    request_line->method = strdup(method);
    request_line->request_target = strdup(target);
    request_line->http_version = strdup(numeric_version);

    if (request_line->method == NULL || request_line->request_target == NULL ||
        request_line->http_version == NULL) {
        free(request_line->method);
        free(request_line->request_target);
        free(request_line->http_version);
        free(request_line);
        free(request_copy);
        return -1;
    }

    out_request->line = request_line;
    free(request_copy);

    return 0;
}

void free_request(request_t *request) {
    if (request == NULL || request->line == NULL) {
        return;
    }

    free(request->line->method);
    free(request->line->request_target);
    free(request->line->http_version);
    free(request->line);
    request->line = NULL;
}
