/* common.cc */

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

namespace {

typedef int (*bind_or_connect_t)(int, const struct sockaddr *, socklen_t);

/**
 * Constructor helper function.
 */
int ctorAux (int flags, const char *node, const char *service,
        bind_or_connect_t func) {
    assert(&::bind == func || &::connect == func);

   /* The following code is largely lifted from the getaddrinfo(3) man page.
    * getaddrinfo provides a handy-dandy way to factor server socket creation
    * and client socket creation into the same common code. */

   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = flags;
   hints.ai_protocol = 0;
   hints.ai_canonname = nullptr;
   hints.ai_addr = nullptr;
   hints.ai_next = nullptr;

   struct addrinfo *results;

   int rc = getaddrinfo(node, service, &hints, &results);
   if (0 != rc) {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
       exit(EXIT_FAILURE);
   }

   struct addrinfo *rp;
   int fd;

   for (rp = results; rp; rp = rp->ai_next) {
       fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
       if (-1 == fd) {
           continue;
       }

       /* We take advantage of the fact that both bind and connect share the
        * same type signature, and both return 0 on success. */
       rc = func(fd, rp->ai_addr, rp->ai_addrlen);
       if (0 == rc) {
           break;
       }
       
       /* We haven't sent anything, so the return value of close is
        * irrelevant. */
       close(fd);
   }

   if (!rp) {
       fprintf(stderr, "Unable to create socket.\n");
       exit(EXIT_FAILURE);
   }

   freeaddrinfo(results);

   return fd;
}


} // file scope namespace

//////////////////////////////////////////////////////////////////////////////

UDPIPv4Socket::UDPIPv4Socket (const char *port)
        : fd(ctorAux(AI_PASSIVE, nullptr, port, &::bind))
        , connected(false) { }

UDPIPv4Socket::UDPIPv4Socket (const char *host, const char *port)
        : fd(ctorAux(0, host, port, &::connect))
        , connected(true) { }

//////////////////////////////////////////////////////////////////////////////

UDPIPv4Socket::~UDPIPv4Socket () {
    int rc = close(fd);
    if (-1 == rc) {
        /* Not much we can do about it here--this is really just a warning. */
        perror("close");
    }
}

//////////////////////////////////////////////////////////////////////////////

void UDPIPv4Socket::send (const char *buf, size_t buflen) const {
    assert(connected);

    ssize_t txlen = ::send(fd, static_cast<const void *>(buf), buflen, 0);

    if (-1 == txlen) {
        perror("send");
    }
}

void UDPIPv4Socket::send (const Address& addr, const char *buf, size_t buflen) const {
    ssize_t txlen = sendto(fd, static_cast<const void *>(buf), buflen, 0,
            addr.getSockaddr(), addr.getSockaddrLen());

    if (-1 == txlen) {
        perror("sendto");
    }
}

bool UDPIPv4Socket::timedRecvFrom (Address& addr, char *buf, size_t& buflen, int microseconds) const {

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = microseconds;

    int rc = select(1, &rfds, nullptr, nullptr, &tv);

    if (-1 == rc) {
        perror("select");
        exit(EXIT_FAILURE);
    }

    if (!rc) {
        /* Timer expired. */
        return false;
    }

    /* We can go ahead and recvfrom. */
    assert(FD_ISSET(fd, &rfds));

    recvFrom(addr, buf, buflen);
    return true;
}

void UDPIPv4Socket::recvFrom (Address& addr, char *buf, size_t& buflen) const {
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
        printf("Remote end performed orderly shutdown.\n");
    }

    addr = Address(src_addr);
    buflen = rxlen;
}

//////////////////////////////////////////////////////////////////////////////

std::string Address::humanReadable () const {
    char hbuf[NI_MAXHOST];
    char sbuf[NI_MAXSERV];

    int rc = getnameinfo(getSockaddr(), getSockaddrLen(), hbuf, sizeof(hbuf),
            sbuf, sizeof(sbuf), NI_NUMERICSERV | NI_NOFQDN);
    if (-1 == rc) {
        perror("getnameinfo");
        exit(EXIT_FAILURE);
    }
    return std::string(hbuf) + ":" + std::string(sbuf);
}
