#include "socket_reader.h"

ssize_t socket_reader(void *context, char *buffer, size_t max_bytes) {
    socket_context_t *ctx = (socket_context_t *)context;
    size_t bytes_received = tcprecv(ctx->socket, buffer, max_bytes, -1);
    if (bytes_received == 0) {
        return 0;
    }

    return (ssize_t)bytes_received;
}
