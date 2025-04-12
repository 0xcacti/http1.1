#include <assert.h>
#include <errno.h>
#include <libdill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Create the IP address structure for localhost:42069
    struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, 42069, 0);
    if (rc < 0) {
        perror("Cannot create address");
        return 1;
    }

    // Create a listening socket
    int ls = tcp_listen(&addr, 10);
    if (ls < 0) {
        perror("Cannot listen");
        return 1;
    }
    printf("Server listening on port 42069\n");

    // Accept a client connection
    int s = tcp_accept(ls, NULL, -1);
    if (s < 0) {
        perror("Cannot accept connection");
        hclose(ls);
        return 1;
    }
    printf("Accepted connection\n");

    // Step 2 from tutorial: Attach a SUFFIX protocol to split the TCP bytestream into messages
    s = suffix_attach(s, "\r\n", 2);
    if (s < 0) {
        perror("Cannot attach suffix protocol");
        return 1;
    }

    // Read data using message-based API (mrecv)
    char buf[1024] = {0};

    // Try to receive a message
    ssize_t bytes = mrecv(s, buf, sizeof(buf) - 1, -1);
    if (bytes >= 0) {
        // Success - we got a message
        buf[bytes] = '\0';
        printf("Received: %s\n", buf);
    } else if (errno == ETIMEDOUT) {
        printf("Timed out waiting for message\n");
    } else {
        perror("mrecv");
    }

    // Close the sockets - hclose works recursively as mentioned in the tutorial
    rc = hclose(s); // This closes both the suffix socket and underlying TCP socket
    assert(rc == 0);

    rc = hclose(ls);
    assert(rc == 0);

    return 0;
}
