#include "internal/request/request.h"
#include "internal/socket_reader/socket_reader.h"
#include <errno.h>
#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 42069
#define BUFFER_SIZE 8

int main(void) {
    // Disable output buffering
    setbuf(stdout, NULL);

    ipaddr addr = iplocal(NULL, PORT, 0);
    tcpsock ls = tcplisten(addr, 10);
    if (!ls) {
        perror("Can't open listening socket");
        return 1;
    }

    msleep(now() + 10);

    printf("Listening for TCP traffic on :%d\n", PORT);

    while (1) {
        tcpsock as = tcpaccept(ls, -1);
        if (!as) {
            continue;
        }

        // Get remote address for logging
        ipaddr remote_addr = tcpaddr(as);
        char addr_str[256];
        ipaddrstr(remote_addr, addr_str);
        printf("Accepted connection from %s\n", addr_str);

        socket_context_t ctx = {.socket = as};
        request_t request;
        int result = request_from_reader(socket_reader, &ctx, &request);
        if (result < 0) {
            fprintf(stderr, "Error reading request: %s\n", strerror(-result));
            tcpclose(as);
            continue;
        }

        printf("Request line:");
        printf("- Method: %s\n", request.request_line->method);
        printf("- Target: %s\n", request.request_line->request_target);
        printf("- Version: %s\n", request.request_line->http_version);
        printf("Headers:\n");
        if (request.headers && request.headers->map) {
            khint_t k;
            for (k = 0; k < kh_end(request.headers->map); ++k) {
                if (kh_exist(request.headers->map, k)) {
                    const char *key = kh_key(request.headers->map, k);
                    const char *value = kh_value(request.headers->map, k);
                    printf("- %s: %s\n", key, value);
                }
            }
        }
        printf("Body:\n");
        if (request.body) {
            printf("%s\n", request.body);
        }
    }

    return 0;
}
