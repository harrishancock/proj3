/* urecv.cc */

#define _BSD_SOURCE

#include "common.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

void usage (const char *program) {
    printf("Usage: %s <port>\n", program);
}

} // file scope namespace

int main (int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    /* Read the port number from the command line. */
    long port;
    {
        char *end;
        errno = 0;
        port = strtol(argv[1], &end, 10);
        if ('\0' != *end || errno || port < 0 || port > 65535) {
            fprintf(stderr, "%s is not a valid port number\n", argv[1]);
            return 1;
        }
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == fd) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    int rc = bind(fd, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
    if (-1 == rc) {
        perror("bind");
        return 1;
    }

    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    memset(&src_addr, 0, addrlen);
    const size_t buflen = 1 << 10;
    unsigned char buf[buflen];

    ssize_t rxlen = recvfrom(fd, static_cast<void *>(buf), buflen, 0,
            reinterpret_cast<struct sockaddr *>(&src_addr), &addrlen);
    if (-1 == rxlen) {
        perror("recvfrom");
        return 1;
    }

    if (0 == rxlen) {
        printf("Remote end performed orderly shutdown.\n");
    }
    else {
        char s[INET_ADDRSTRLEN];
        printf("received %zd bytes from %s:%d\n", rxlen, inet_ntop(AF_INET, &src_addr.sin_addr, s, INET_ADDRSTRLEN),
                ntohs(src_addr.sin_port));
    }

    rc = close(fd);
    if (-1 == rc) {
        perror("close");
        return 1;
    }

    return 0;
}
