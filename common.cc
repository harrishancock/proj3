/* common.cc */

/* 
 * Hancock, Harris
 * CS 3590
 * 8 December 2012
 * Project #3: Write a file transfer client/server.
 */

#include "common.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>

//////////////////////////////////////////////////////////////////////////////

std::string timestring (const char *format) {
    char s[200];

    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    if (!tmp) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }
    
    int rc = strftime(s, sizeof(s), format, tmp);
    if (0 == rc) {
        fprintf(stderr, "strftime failed\n");
        exit(EXIT_FAILURE);
    }

    return std::string(s);
}

//////////////////////////////////////////////////////////////////////////////

UDPIPv4Socket::UDPIPv4Socket (uint16_t port)
        : fd(0) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == fd) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    
    int rc = bind(fd, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
    if (-1 == rc) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

//////////////////////////////////////////////////////////////////////////////

UDPIPv4Socket::~UDPIPv4Socket () {
    int rc = close(fd);
    if (-1 == rc) {
        /* Not much we can do about it here--this is really just a warning. */
        perror("close");
    }
}

//////////////////////////////////////////////////////////////////////////////

void UDPIPv4Socket::send (const IPv4Address& addr,
        const char *buf, size_t buflen) const {
    ssize_t txlen = sendto(fd, static_cast<const void *>(buf), buflen, 0,
            addr.getSockaddr(), addr.getSockaddrLen());

    if (-1 == txlen) {
        perror("sendto");
    }
}

bool UDPIPv4Socket::timedRecv (IPv4Address& addr,
        char *buf, size_t& buflen, int seconds) const {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    int rc = select(fd + 1, &rfds, NULL, NULL, &tv);

    if (-1 == rc) {
        perror("select");
        exit(EXIT_FAILURE);
    }

    if (!rc) {
        /* Timer expired. */
        return false;
    }

    /* No timer expired--we can go ahead and recvfrom. */
    assert(FD_ISSET(fd, &rfds));

    recv(addr, buf, buflen);
    return true;
}

void UDPIPv4Socket::recv (IPv4Address& addr, char *buf, size_t& buflen) const {
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    memset(&src_addr, 0, addrlen);

    ssize_t rxlen = recvfrom(fd, static_cast<void *>(buf), buflen, 0,
            reinterpret_cast<struct sockaddr *>(&src_addr), &addrlen);

    if (-1 == rxlen) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    if (0 == rxlen) {
        /* Does this even happen with UDP? */
        printf("Remote end performed orderly shutdown.\n");
    }

    /* Set the output parameters. */
    addr = IPv4Address(src_addr);
    buflen = rxlen;
}

//////////////////////////////////////////////////////////////////////////////

IPv4Address::IPv4Address (const char *host, uint16_t port) {
    struct hostent *hp;
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "gethostbyname: %s\n", hstrerror(h_errno));
        exit(EXIT_FAILURE);
    }

    sin.sin_family = AF_INET;
    memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_port = htons(port);
}

//////////////////////////////////////////////////////////////////////////////

std::string IPv4Address::humanReadable () const {
    char hbuf[NI_MAXHOST];
    char sbuf[NI_MAXSERV];

    /* The two flags passed here tell getnameinfo always to return the service
     * as a numeric port string, and not to fully-qualify host names on the
     * local domain (otherwise, you'd see things like
     * "localhost.localdomain", which is kind of annoying). */
    int rc = getnameinfo(getSockaddr(), getSockaddrLen(), hbuf, sizeof(hbuf),
            sbuf, sizeof(sbuf), NI_NUMERICSERV | NI_NOFQDN);
    if (-1 == rc) {
        perror("getnameinfo");
        exit(EXIT_FAILURE);
    }
    return std::string(hbuf) + ":" + std::string(sbuf);
}
