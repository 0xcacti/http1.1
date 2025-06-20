#ifndef TCPLISTENER_SOCKET_READER_H
#define TCPLISTENER_SOCKET_READER_H

#include <sys/types.h>
#include <stddef.h>
#include <libmill.h>

typedef struct {
    tcpsock socket;
} socket_context_t;

ssize_t socket_reader(void *context, char *buffer, size_t max_bytes);
#endif

