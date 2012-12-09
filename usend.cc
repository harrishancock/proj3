/* usend.cc */

#include "common.hh"

#include <cstdio>
#include <cassert>

#include <string>

namespace {

void usage (const char *program) {
    printf("Usage: %s <host> <port> <file>\n", program);
}

} // file scope namespace

int main (int argc, char **argv) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    UDPIPv4Socket sock (argv[1], argv[2]);

    sock.send(argv[3], strlen(argv[3]));

    Address addr;
    const size_t buflen = 1 << 10;
    char buf[buflen + 1];
    size_t rlen = buflen;
    sock.recvFrom(addr, buf, rlen);
    printf("recv\n");
    assert(51 == rlen);

    /* Store the next sequence number and return the current one as an ACK. */
    char seq = buf[0] + 1;
    sock.send(buf, 1);
    printf("send\n");

    while (true) {
        Address next_addr;
        rlen = buflen;
        sock.recvFrom(next_addr, buf, rlen);
        printf("recv\n");

        if (next_addr != addr) {
            /* Ignore packets from any other address. */
            continue;
        }

        if (buf[0] != seq) {
            /* Ignore out-of-sequence packets. */
            continue;
        }

        if (51 != rlen) {
            /* File transfer complete. */
            break;
        }

        seq++;
        sock.send(buf, 1);
        printf("send\n");
    }

    return 0;
}
