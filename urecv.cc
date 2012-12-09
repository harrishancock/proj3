/* urecv.cc */

/* 
 * Hancock, Harris
 * CS 3590
 * 8 December 2012
 * Project #3: Write a file transfer client/server.
 */

#include "common.hh"

#include <unistd.h>

#include <cstdio>
#include <cassert>

#include <fstream>
#include <memory>

#define TIMEOUT 20  // milliseconds

namespace {

void usage (const char *program) {
    printf("Usage: %s <port>\n", program);
}

void print_sent (const char *buf) {
    printf("Server sent frame: Sequence number = %hhd, Time = %s, "
            "Packet contents = %c%c%c%c%c\n",
            buf[0], timestring("%T").c_str(),
            buf[1], buf[2], buf[3], buf[4], buf[5]);
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
    const size_t buflen = PAYLOADLEN + 1;   /* +1 for header */
    char buf[buflen + 1];  /* +1 so we can always append a null */
    size_t rlen = buflen;

    /* The first message we receive is the file name requested. */
    sock.recv(addr, buf, rlen);
    buf[rlen] = '\0';

    printf("%s requesting file %s\n", addr.humanReadable().c_str(), buf);

    std::ifstream f (buf, std::ios::in | std::ios::ate);
    if (!f.good()) {
        printf("File not available\n");
        return 1;
    }

    /* Read the whole file all at once. */
    ssize_t filesize = f.tellg();
    char *fmem = new char [filesize];
    f.seekg(0, std::ios::beg);
    f.read(fmem, filesize);
    f.close();

    ssize_t filepos = 0;
    char seq = 0;

    /* Loop only while we have frames to send. A zero-length payload is valid,
     * as this is the only way to signify the end of a file to the client, if
     * the file is a multiple of PAYLOADLEN. */
    while (0 <= filesize - filepos) {
        buf[0] = seq;

        rlen = std::min(filesize - filepos, static_cast<ssize_t>(PAYLOADLEN));
        memcpy(buf + 1, fmem + filepos, rlen);
        /* Don't increment filepos yet--only after we get an ACK. */

        sock.send(addr, buf, rlen + 1);
        print_sent(buf);

        /* The sleep here counts towards the 2-second timeout. */
        sleep(1);

        IPv4Address next_addr;
        rlen = buflen;

        /* We'll need to loop to account for any spurious packets we might
         * receive from other hosts. Note that this causes a small bug: in the
         * unlikely event that our process receives several spurious packets,
         * we still wait one second for each recv, potentially expanding the
         * perceived timeout value. Working around this isn't too difficult,
         * but would probably destroy any readability this code has left, so
         * I'll just live with it. */
        bool rx = false;
        do {
            /* Specify only a 1-second timeout, to account for the sleep above. */
            rx = sock.timedRecv(next_addr, buf, rlen, 1);
            if (rx && next_addr != addr) {
                printf("packet arrived from incorrect address\n");
            }
        } while (rx && next_addr != addr);

        /* Check to see if we timed out. */
        if (!rx) {
            print_timeout(buf[0]);
            continue;
        }

        assert(rlen == 1);     /* it must be an ACK */
        assert(buf[0] == seq); /* we're assuming the client's ACKs are reliable */

        print_receipt(buf[0]);

        seq++;
        filepos += PAYLOADLEN;
    }

    delete fmem;
    printf("File transfer complete.\n");

    return 0;
}
