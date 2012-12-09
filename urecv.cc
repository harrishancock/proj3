/* urecv.cc */

#include "common.hh"

#include <cstdio>

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

    UDPIPv4Socket sock (argv[1]);

    Address addr;
    const size_t buflen = 1 << 10;
    unsigned char buf[buflen];
    size_t rlen = buflen;

    sock.recvFrom(addr, buf, rlen);

    return 0;
}
