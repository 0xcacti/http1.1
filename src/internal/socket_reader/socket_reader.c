#include <internal/socket_reader/socket_reader.h>

ssize_t socket_reader(void *context, char *buffer, size_t max_bytes) {
    socket_context_t *ctx = (socket_context_t *)context;

    // Use a small timeout instead of infinite
    size_t bytes_received = tcprecv(ctx->socket, buffer, max_bytes, now() + 1);

    if (bytes_received == 0 && errno == ETIMEDOUT) {
        // No data available - this is like EOF in the Go version
        return 0;
    }

    return (ssize_t)bytes_received;
}
