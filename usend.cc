/* usend.cc */

#define _BSD_SOURCE

#include "common.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <cstdio>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

void usage (const char *program) {
    printf("Usage: %s <host> <port>\n", program);
}

} // file scope namespace

int main (int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    /* Look up the supplied host. */
    struct addrinfo *results;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int rc = getaddrinfo(argv[1], argv[2], &hints, &results);
    if (0 != rc) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

#if 0
    /* Read the port number from the command line. */
    long port;
    {
        char *end;
        errno = 0;
        port = strtol(argv[2], &end, 10);
        if ('\0' != *end || errno || port < 0 || port > 65535) {
            fprintf(stderr, "%s is not a valid port number\n", argv[1]);
            return 1;
        }
    }
#endif

    struct addrinfo *rp;
    int fd;

    for (rp = results; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (-1 == fd) {
            continue;
        }

        rc = connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (-1 == rc) {
            break;
        }

        close(fd);
    }

    freeaddrinfo(results);

    if (nullptr == rp) {
        fprintf(stderr, "Unable to connect.\n");
        return 1;
    }

    rp = nullptr;

    const char *buf = "Hello, world!";
    const size_t buflen = strlen(buf);

    ssize_t txlen = send(fd, static_cast<const void *>(buf), buflen, 0);

    if (-1 == txlen) {
        perror("send");
        return 1;
    }
    printf("sent %zd bytes\n", txlen);
#if 0
    printf("sent %zd bytes to %s:%d\n", txlen, inet_ntop(results->ai_addr),
            ntohs(src_addr.sin_port));
#endif

    rc = close(fd);
    if (-1 == rc) {
        perror("close");
        return 1;
    }

    freeaddrinfo(results);

    return 0;
}
