/* usend.cc */

#include "common.hh"

#include <cstdio>
#include <cassert>

#include <string>

namespace {

void usage (const char *program) {
    printf("Usage: %s <host> <port> <file>\n", program);
}

void print_receipt (const char *buf) {
    printf("Client received frame: Sequence number = %hhd, Time = %s, Packet contents = %c%c%c%c%c\n",
            buf[0], timestring("%T").c_str(), buf[1], buf[2], buf[3], buf[4], buf[5]);
}

void print_sent (char seq) {
    printf("Client sent ACK: Sequence number = %hhd, Time = %s\n", seq, timestring("%T").c_str());
}

void print_discard (char seq) {
    printf("Client discarding frame: Sequence number = %hhd, Time = %s\n", seq, timestring("%T").c_str());
}

void unreliableRecv (const UDPIPv4Socket& sock, IPv4Address& addr,
        char *buf, size_t& buflen) {
    int rxd_packets = 0;

    size_t rlen;
    do {
        if (rxd_packets) {
            print_discard(buf[0]);
        }
        rlen = buflen;
        sock.recv(addr, buf, rlen);
        rxd_packets++;
    } while (!(rand() % 4));

    buflen = rlen;
}

} // file scope namespace

int main (int argc, char **argv) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    UDPIPv4Socket sock (argv[1], argv[2]);

    sock.send(argv[3], strlen(argv[3]));

    IPv4Address addr;
    const size_t buflen = 1 << 10;
    char buf[buflen + 1];
    size_t rlen = buflen;
    sock.recv(addr, buf, rlen);
    assert(51 == rlen);
    print_receipt(buf);

    /* Store the next sequence number and return the current one as an ACK. */
    char seq = buf[0] + 1;
    sock.send(buf, 1);
    print_sent(buf[0]);

    while (true) {
        IPv4Address next_addr;
        rlen = buflen;
        unreliableRecv(sock, next_addr, buf, rlen);
        print_receipt(buf);

        if (next_addr != addr) {
            printf("packet from other address\n");
            continue;
        }

        if (buf[0] != seq) {
            printf("out-of-sequence packet\n");
            continue;
        }

        if (51 != rlen) {
            printf("file transfer complete\n");
            break;
        }

        seq++;
        sock.send(buf, 1);
        print_sent(buf[0]);
    }

    return 0;
}
