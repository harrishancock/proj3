/* common.hh */

#ifndef CS3590_PROJ3_COMMON_HH
#define CS3590_PROJ3_COMMON_HH

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstring>

#include <string>

#define PAYLOADLEN 50

//////////////////////////////////////////////////////////////////////////////

/**
 * Return a string representation of the time now. The format parameter gets
 * passed to strftime--see the man page.
 */
std::string timestring (const char *format);

//////////////////////////////////////////////////////////////////////////////

/**
 * Container for an IPv4 sockaddr_in data structure.
 */
class IPv4Address {
public:
    IPv4Address () {
        memset(&sin, 0, sizeof(sin));
    }

    explicit IPv4Address (struct sockaddr_in s)
            : sin(s) { }

    const struct sockaddr *getSockaddr () const {
        return reinterpret_cast<const struct sockaddr *>(&sin);
    }

    socklen_t getSockaddrLen () const {
        return sizeof(sin);
    }

    /**
     * Address in host:port string format.
     */
    std::string humanReadable () const;

    bool operator== (const IPv4Address& other) {
        return sin.sin_addr.s_addr == other.sin.sin_addr.s_addr
            && sin.sin_port == other.sin.sin_port;
    }

    bool operator!= (const IPv4Address& other) {
        return !(*this == other);
    }

private:
    struct sockaddr_in sin;
};

//////////////////////////////////////////////////////////////////////////////

/**
 * Abstraction of both server and client sockets.
 */
class UDPIPv4Socket {
public:
    /* non-copyable */
    UDPIPv4Socket (const UDPIPv4Socket&) = delete;
    UDPIPv4Socket& operator= (const UDPIPv4Socket&) = delete;

    /**
     * Create a passive socket bound to INADDR_ANY on the port specified (i.e.,
     * a server socket).
     */
    explicit UDPIPv4Socket (const char *port);

    /**
     * Create a socket "connected" to the host/port specified (i.e., a client
     * socket).
     */
    UDPIPv4Socket (const char *host, const char *port);

    ~UDPIPv4Socket ();

    bool isConnected () const {
        return connected;
    }

    /**
     * Send a buffer over a "connected" UDP socket. Primarily used by the
     * client.
     */
    void send (const char *buf, size_t buflen) const;

    /**
     * Send a buffer over a UDP socket to an arbitrary address.
     */
    void send (const IPv4Address& addr, const char *buf, size_t buflen) const;

    /**
     * Block until a packet is received, or a timer expires. Return true if a
     * packet has been successfully received, false if the timer expired.
     * buflen is an in/out parameter--it should initially be set to the length
     * of the buffer pointed to by buf; on return it will hold the actual
     * number of bytes written into buf.
     */
    bool timedRecv (IPv4Address& addr, char *buf, size_t& buflen,
            int seconds) const;

    /**
     * Block until a packet is received. buflen has the same semantics as in
     * timedRecv.
     */
    void recv (IPv4Address& addr, char *buf, size_t& buflen) const;

private:
    int fd;
    bool connected;
};

#endif
