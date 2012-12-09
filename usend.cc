/* usend.cc */

#include "common.hh"

#include <cstdio>

#include <string>

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

    UDPIPv4Socket sock (argv[1], argv[2]);

    std::string hworld ("Hello, world!");
    sock.send(reinterpret_cast<const unsigned char *>(hworld.c_str()), hworld.size());
#if 0

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
#endif

    return 0;
}
