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

    UDPIPv4Socket sock (0);
    IPv4Address dest (host, atoi(port));

    printf("Requesting file %s from %s:%s\n", file, host, port);
    const size_t buflen = 1024;
    char buf[buflen];
    strncpy(buf, file, buflen);
    sock.send(dest, buf, buflen);

    IPv4Address addr;
    const size_t framelen = PAYLOADLEN + 1;   /* +1 for header */
    char frame[framelen];

    size_t rlen;
    char seq = 0;   /* Arbitrarily decide that 0 must be the first seqno. */

    do {
        /* rlen will be used as an output parameter, so copy framelen. */
        rlen = framelen;

        /* Wait for the next frame. */
        unreliableRecv(sock, addr, frame, rlen);

        if (addr != dest) {
            printf("packet arrived from incorrect address\n");
            continue;
        }

        if (frame[0] != seq) {
            printf("out-of-sequence packet\n");
            continue;
        }

        print_receipt(frame);

        /* Seems to check out okay--write it. */
        f.write(frame + 1, rlen - 1);

        /* ACK the previous frame. Note that it would be nice if we could send
         * only the single first byte of the frame--i.e., the part containing
         * the sequence number. This works on most machines I tested against,
         * but the College of Science machines require a minimum buffer size of
         * 12 before they will deign to actually send a packet to a non-
         * loopback host, so just parrot the entire frame back to the server to
         * play it safe. */
        sock.send(dest, frame, framelen);
        print_sent(frame[0]);

        seq++;

        /* Only loop if we received a fully-loaded frame. */
    } while (PAYLOADLEN + 1 == rlen);

    f.close();
    printf("File transfer complete.\n");

    return 0;
}
