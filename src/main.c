#include <errno.h>
#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 42069
#define BUFFER_SIZE 1024
#define CONNECTION_BACKLOG 10

coroutine void dialogue(tcpsock as) {
    tcpsend(as, "What's your name?\r\n", 19, -1);
    if (errno != 0)
        goto cleanup;
    tcpflush(as, -1);
    if (errno != 0)
        goto cleanup;

    char inbuf[256];
    size_t sz = tcprecvuntil(as, inbuf, sizeof(inbuf), "\r", 1, -1);
    if (errno != 0)
        goto cleanup;

    inbuf[sz - 1] = 0;
    char outbuf[256];
    int rc = snprintf(outbuf, sizeof(outbuf), "Hello, %s!\r\n", inbuf);

    sz = tcpsend(as, outbuf, rc, -1);
    if (errno != 0)
        goto cleanup;
    tcpflush(as, -1);
    if (errno != 0)
        goto cleanup;

cleanup:
    tcpclose(as);
}

int main(void) {
    ipaddr addr = iplocal(NULL, PORT, 0);
    tcpsock ls = tcplisten(addr, 10);
    if (!ls) {
        perror("Can't open listening socket");
        return 1;
    }

    while (1) {
        tcpsock as = tcpaccept(ls, -1);
        if (!as)
            continue;
        go(dialogue(as));
    }
    return 0;
}
