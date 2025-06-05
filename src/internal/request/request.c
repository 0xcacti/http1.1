#include <ctype.h>
#include <internal/request/request.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

const int BUFFER_SIZE = 8;

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

int find_crlf(const char *data, size_t length) {
    if (data == NULL || length < 2) {
        return -1;
    }

    for (size_t i = 0; i < length - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }

    return -1;
}

int parse_request_line(const char *data, size_t length, request_t *out_request) {

    if (data == NULL || out_request == NULL || length == 0) {
        return -1;
    }

    int crlf_index = find_crlf(data, length);
    if (crlf_index < 0) {
        return 0;
    }

    char *line = malloc(crlf_index + 1);
    if (line == NULL) {
        return -1;
    }
    memcpy(line, data, crlf_index);
    line[crlf_index] = '\0';

    char *field_saveptr;
    char *method = strtok_r(line, " ", &field_saveptr);
    char *target = strtok_r(NULL, " ", &field_saveptr);
    char *version = strtok_r(NULL, " ", &field_saveptr);

    if (method == NULL || target == NULL || version == NULL) {
        free(line);
        return -1;
    }

    if (!is_alphabetic_uppercase(method)) {
        free(line);
        return -1;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        free(line);
        return -1;
    }

    char *numeric_version = version + 5; // Skip "HTTP/"

    request_line_t *request_line = malloc(sizeof(request_line_t));
    if (request_line == NULL) {
        free(line);
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
        return -1;
    }

    out_request->request_line = request_line;
    free(line);
    return crlf_index + 2;
}

int parse(const char *data, size_t length, request_t *out_request) {
    if (data == NULL || out_request == NULL || length == 0) {
        return -1;
    }

    switch (out_request->state) {
    case INITIALIZED: {
        int bytes_parsed = parse_request_line(data, length, out_request);
        if (bytes_parsed < 0) {
            return -1;
        }
        if (bytes_parsed == 0) {
            return 0;
        }
        out_request->state = DONE;
        return bytes_parsed;
    }
    case DONE:
        return -1;
    default:
        return -1; // Invalid state
    }
}

int request_from_reader(reader_func_t reader, void *read_context, request_t *out_request) {
    size_t buffer_capacity = BUFFER_SIZE;
    char *buffer = malloc(buffer_capacity);
    if (buffer == NULL) {
        return -1;
    }
    size_t read_to_index = 0;
    out_request->state = INITIALIZED;

    while (out_request->state != DONE) {
        if (read_to_index >= buffer_capacity) {
            buffer_capacity *= 2;
            char *new_buffer = realloc(buffer, buffer_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return -1;
            }
            buffer = new_buffer;
        }
        ssize_t bytes_read =
            reader(read_context, buffer + read_to_index, buffer_capacity - read_to_index);

        if (bytes_read < 0) {
            free(buffer);
            return -1;
        }

        if (bytes_read == 0) {
            out_request->state = DONE;
        }
        read_to_index += bytes_read;
        int bytes_parsed = parse(buffer, read_to_index, out_request);
        if (bytes_parsed < 0) {
            free(buffer);
            return -1;
        }
        if (bytes_parsed > 0) {
            memmove(buffer, buffer + bytes_parsed, read_to_index);
            read_to_index -= bytes_parsed;
        }
    }
    free(buffer);
    return 0;
}

void free_request(request_t *request) {
    if (request == NULL || request->request_line == NULL) {
        return;
    }

    free(request->request_line->method);
    free(request->request_line->request_target);
    free(request->request_line->http_version);
    free(request->request_line);
    request->request_line = NULL;
}
#include <internal/request/request.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

const int BUFFER_SIZE = 8;

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

int find_crlf(const char *data, size_t length) {
    if (data == NULL || length < 2) {
        return -1;
    }

    for (size_t i = 0; i < length - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }

    return -1;
}

int parse_request_line(const char *data, size_t length, request_t *out_request) {

    if (data == NULL || out_request == NULL || length == 0) {
        return -1;
    }

    int crlf_index = find_crlf(data, length);
    if (crlf_index < 0) {
        return 0;
    }

    char *line = malloc(crlf_index + 1);
    if (line == NULL) {
        return -1;
    }
    memcpy(line, data, crlf_index);
    line[crlf_index] = '\0';

    char *field_saveptr;
    char *method = strtok_r(line, " ", &field_saveptr);
    char *target = strtok_r(NULL, " ", &field_saveptr);
    char *version = strtok_r(NULL, " ", &field_saveptr);

    if (method == NULL || target == NULL || version == NULL) {
        free(line);
        return -1;
    }

    if (!is_alphabetic_uppercase(method)) {
        free(line);
        return -1;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        free(line);
        return -1;
    }

    char *numeric_version = version + 5; // Skip "HTTP/"

    request_line_t *request_line = malloc(sizeof(request_line_t));
    if (request_line == NULL) {
        free(line);
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
        return -1;
    }

    out_request->request_line = request_line;
    free(line);
    return crlf_index + 2;
}

int parse(const char *data, size_t length, request_t *out_request) {
    if (data == NULL || out_request == NULL || length == 0) {
        return -1;
    }

    switch (out_request->state) {
    case INITIALIZED: {
        int bytes_parsed = parse_request_line(data, length, out_request);
        if (bytes_parsed < 0) {
            return -1;
        }
        if (bytes_parsed == 0) {
            return 0;
        }
        out_request->state = DONE;
        return bytes_parsed;
    }
    case DONE:
        return -1;
    default:
        return -1; // Invalid state
    }
}

int request_from_reader(reader_func_t reader, void *read_context, request_t *out_request) {
    size_t buffer_capacity = BUFFER_SIZE;
    char *buffer = malloc(buffer_capacity);
    if (buffer == NULL) {
        return -1;
    }
    size_t read_to_index = 0;
    out_request->state = INITIALIZED;

    while (out_request->state != DONE) {
        if (read_to_index >= buffer_capacity) {
            buffer_capacity *= 2;
            char *new_buffer = realloc(buffer, buffer_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return -1;
            }
            buffer = new_buffer;
        }
        ssize_t bytes_read =
            reader(read_context, buffer + read_to_index, buffer_capacity - read_to_index);

        if (bytes_read < 0) {
            free(buffer);
            return -1;
        }

        if (bytes_read == 0) {
            out_request->state = DONE;
        }
        read_to_index += bytes_read;
        int bytes_parsed = parse(buffer, read_to_index, out_request);
        if (bytes_parsed < 0) {
            free(buffer);
            return -1;
        }
        if (bytes_parsed > 0) {
            memmove(buffer, buffer + bytes_parsed, read_to_index);
            read_to_index -= bytes_parsed;
        }
    }
    free(buffer);
    return 0;
}

void free_request(request_t *request) {
    if (request == NULL || request->request_line == NULL) {
        return;
    }

    free(request->request_line->method);
    free(request->request_line->request_target);
    free(request->request_line->http_version);
    free(request->request_line);
    request->request_line = NULL;
}
