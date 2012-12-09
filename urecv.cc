/* urecv.cc */

#include "common.hh"

#include <unistd.h>

#include <cstdio>
#include <cassert>

#include <fstream>

#define TIMEOUT 20  // milliseconds

namespace {

void usage (const char *program) {
    printf("Usage: %s <port>\n", program);
}

void print_sent (const char *buf) {
    printf("Server sent frame: Sequence number = %hhd, Time = %s, Packet contents = %c%c%c%c%c\n",
            buf[0], timestring("%T").c_str(), buf[1], buf[2], buf[3], buf[4], buf[5]);
}

void print_receipt (char seq) {
    printf("Server received ACK: Sequence number = %hhd, Time = %s\n",
            seq, timestring("%T").c_str());
}

void print_timeout (char seq) {
    printf("Server timeout: Sequence number = %hhd, Time = %s\n",
            seq, timestring("%T").c_str());
}

} // file scope namespace

//////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    UDPIPv4Socket sock (argv[1]);

    IPv4Address addr;
    const size_t buflen = 1 << 10;
    char buf[buflen + 1];  /* +1 so we can always append a null */
    size_t rlen = buflen;

    /* The first message we receive is the file name requested. */
    sock.recv(addr, buf, rlen);
    buf[rlen] = '\0';

    printf("%s requesting %s\n", addr.humanReadable().c_str(), buf);

    std::ifstream f (buf);
    if (!f.good()) {
        printf("File not available\n");
        return 1;
    }

    char seq = 0;
    buf[0] = seq;
    f.read(buf + 1, 50);
    while (!f.eof()) {
        sock.send(addr, buf, 51);
        print_sent(buf);
        sleep(1);

        IPv4Address next_addr;
        rlen = buflen;
        bool rx = sock.timedRecv(next_addr, buf, rlen, 1);
        if (!rx) {
            print_timeout(buf[0]);
            continue;
        }
        print_receipt(buf[0]);

        assert(next_addr == addr);
        assert(rlen == 1);
        assert(buf[0] == seq);

        seq++;
        buf[0] = seq;
        f.read(buf + 1, 50);
    }

    return 0;
}
