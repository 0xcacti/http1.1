#include <internal/request/request.h>

typedef struct request_line {
    char *http_version;
    char *request_target;
    char *method;
} request_line_t;

typedef struct request {
    request_line_t *line;
} request_t;

int request_init(void) {
    return 0;
}
