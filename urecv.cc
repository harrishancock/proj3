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
#include <cstdlib>
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

    UDPIPv4Socket sock (atoi(argv[1]));

    IPv4Address addr;
    IPv4Address client;
    const size_t framelen = PAYLOADLEN + 1;   /* +1 for header */
    char frame[framelen + 1];  /* +1 so we can always append a null */
    size_t rlen = framelen;

    /* The first message we receive is the file name requested. */
    sock.recv(client, frame, rlen);
    frame[rlen] = '\0';

    printf("%s requesting file %s\n", client.humanReadable().c_str(), frame);

    std::ifstream f (frame, std::ios::in | std::ios::ate);
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
    char seq = 0;   /* The client is expecting 0 as the first seqno. */

    /* Loop only while we have frames to send. A zero-length payload is valid,
     * as this is the only way to signify the end of a file to the client, if
     * the file is a multiple of PAYLOADLEN. (Note that payloads between 0 and
     * 10 bytes will not actually work on the College of Science machines, due
     * to the buffer size restriction I mentioned in the comments in usend.cc.
     * To solve this problem, we could devise a more robust protocol with
     * opcodes to signify our intent to the client.) */
    while (0 <= filesize - filepos) {
        frame[0] = seq;

        rlen = std::min(filesize - filepos, static_cast<ssize_t>(PAYLOADLEN));
        memcpy(frame + 1, fmem + filepos, rlen);
        /* Don't increment filepos yet--only after we get an ACK. */

        sock.send(client, frame, rlen + 1);
        print_sent(frame);

        /* The sleep here counts towards the 2-second timeout. */
        sleep(1);

        rlen = framelen;

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
            rx = sock.timedRecv(addr, frame, rlen, 1);
            if (rx && addr != client) {
                printf("packet arrived from incorrect address\n");
            }
        } while (rx && addr != client);

        /* Check to see if we timed out. */
        if (!rx) {
            print_timeout(frame[0]);
            continue;
        }

        assert(frame[0] == seq); /* we're assuming the client's ACKs are reliable */

        print_receipt(frame[0]);

        seq++;
        filepos += PAYLOADLEN;
    }

    delete fmem;
    printf("File transfer complete.\n");

    return 0;
}
