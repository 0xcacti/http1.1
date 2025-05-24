#include <errno.h>
#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    ipaddr remote_addr = ipremote("127.0.0.1", 42069, 0, -1);
    if (errno != 0) {
        perror("ipremote");
        return 1;
    }

    ipaddr local_addr = iplocal(NULL, 0, 0);
    if (errno != 0) {
        perror("iplocal");
        return 1;
    }

    udpsock sock = udplisten(local_addr);
    if (!sock) {
        perror("udplisten");
        return 1;
    }

    char buffer[1024];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(buffer, sizeof(buffer), stdin)) {
            break;
        }

        size_t len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        udpsend(sock, remote_addr, buffer, len);
        if (errno != 0) {
            perror("udpsend");
        }
    }

    udpclose(sock);
    return 0;
}
