/* usend.cc */

/* 
 * Hancock, Harris
 * CS 3590
 * 8 December 2012
 * Project #3: Write a file transfer client/server.
 */

#include "common.hh"

#include <cstdio>
#include <cassert>
#include <cstdlib>

#include <string>
#include <fstream>

namespace {

void usage (const char *program) {
    printf("Usage: %s <host> <port> <file>\n", program);
}

void print_receipt (const char *buf) {
    printf("Client received frame: Sequence number = %hhd, Time = %s, "
            "Packet contents = %c%c%c%c%c\n",
            buf[0], timestring("%T").c_str(),
            buf[1], buf[2], buf[3], buf[4], buf[5]);
}

void print_sent (char seq) {
    printf("Client sent ACK: Sequence number = %hhd, Time = %s\n",
            seq, timestring("%T").c_str());
}

void print_discard (char seq) {
    printf("Client discarding frame: Sequence number = %hhd, Time = %s\n",
            seq, timestring("%T").c_str());
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
    srand(time(NULL));

    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    const char *host = argv[1];
    const char *port = argv[2];
    const char *file = argv[3];

    std::ofstream f ((std::string(file) + ".out").c_str());
    if (!f.good()) {
        fprintf(stderr, "Unable to open file %s.out for writing.\n", file);
        exit(EXIT_FAILURE);
    }

    printf("Connecting to %s:%s\n", host, port);
    UDPIPv4Socket sock (host, port);

    printf("Requesting file %s\n", file);
    sock.send(file, strlen(file));

    IPv4Address addr;
    const size_t buflen = PAYLOADLEN + 1;   /* +1 for header */
    char buf[buflen];

    /* rlen will be used as an output parameter, so copy buflen. */
    size_t rlen = buflen;
    sock.recv(addr, buf, rlen);
    print_receipt(buf);

    f.write(buf + 1, rlen - 1);

    char seq = buf[0];

    /* Only loop if we received a fully-loaded frame. */
    while (PAYLOADLEN + 1 == rlen) {
        /* ACK the previous frame. */
        sock.send(buf, 1);
        print_sent(buf[0]);

        /* Wait for the next frame. */
        seq++;
        IPv4Address next_addr;
        rlen = buflen;
        unreliableRecv(sock, next_addr, buf, rlen);

        if (next_addr != addr) {
            printf("packet arrived from incorrect address\n");
            continue;
        }

        if (buf[0] != seq) {
            printf("out-of-sequence packet\n");
            continue;
        }

        print_receipt(buf);

        /* Seems to check out okay--write it. */
        f.write(buf + 1, rlen - 1);
    }

    /* The last ACK. */
    sock.send(buf, 1);
    print_sent(buf[0]);

    f.close();
    printf("File transfer complete.\n");

    return 0;
}
