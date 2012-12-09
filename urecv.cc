/* urecv.cc */

#include "common.hh"

#include <cstdio>

#include <fstream>

namespace {

void usage (const char *program) {
    printf("Usage: %s <port>\n", program);
}

} // file scope namespace

//////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    UDPIPv4Socket sock (argv[1]);

    Address addr;
    const size_t buflen = 1 << 10;
    char buf[buflen + 1];  /* +1 so we can always append a null */
    size_t rlen = buflen;

    /* The first message we receive is the file name requested. */
    sock.recvFrom(addr, buf, rlen);
    printf("recv\n");
    buf[rlen] = '\0';

    printf("%s requesting %s\n", addr.humanReadable().c_str(), buf);

    std::ifstream f (buf);
    if (!f.good()) {
        printf("File not available\n");
        return 1;
    }

    char seq = 0;
    while (!f.eof()) {
        buf[0] = seq;
        f.read(buf + 1, 50);
        sock.send(addr, buf, 51);
        printf("send\n");

        Address next_addr;
        rlen = buflen;
        sock.recvFrom(next_addr, buf, rlen);
        printf("recv\n");
        
        if (next_addr != addr) {
            continue;
        }

        if (1 != rlen) {
            continue;
        }

        seq++;
    }

    return 0;
}
