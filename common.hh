/* common.hh */

#ifndef CS3590_PROJ3_COMMON_HH
#define CS3590_PROJ3_COMMON_HH

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstring>

#include <string>

//////////////////////////////////////////////////////////////////////////////

class Address {
public:
    Address () {
        memset(&sin, 0, sizeof(sin));
    }

    explicit Address (struct sockaddr_in s)
            : sin(s) { }

    const struct sockaddr *getSockaddr () const {
        return reinterpret_cast<const struct sockaddr *>(&sin);
    }

    socklen_t getSockaddrLen () const {
        return sizeof(sin);
    }

    std::string humanReadable () const;

    bool operator== (const Address& other) {
        return sin.sin_addr.s_addr == other.sin.sin_addr.s_addr
            && sin.sin_port == other.sin.sin_port;
    }

    bool operator!= (const Address& other) {
        return !(*this == other);
    }

private:
    struct sockaddr_in sin;
};

//////////////////////////////////////////////////////////////////////////////

class Frame {
public:
    /* non-copyable for now */
    Frame (const Frame&) = delete;
    Frame& operator= (const Frame&) = delete;

    /*
     * Various accessors. The data accessors are intended for use by
     * UDPIPv4Socket for sending over the socket API. The payload accessors are
     * intended for use by the user of UDPIPv4Socket.
     */

    const unsigned char *dataPtr () const {
        return data;
    }

    size_t dataLen () const {
        return PAYLOADLEN + 1;
    }

    const unsigned char *payloadPtr () const {
        return data + 1;
    }

    size_t payloadLen () const {
        return PAYLOADLEN;
    }

    unsigned char sequence () const {
        return data[0];
    }

    /*
     * Various mutators.
     */

    /**
     * Copy the supplied buffer into this frame's buffer. An implicit size of
     * PAYLOADLEN + 1 is assumed.
     */
    void setData (const unsigned char *dat) {
        memcpy(data, dat, dataLen());
    }

    /**
     * Copy the supplied payload into this frame's buffer. An implicit buffer
     * size of PAYLOADLEN is assumed.
     */
    void setPayload (const unsigned char *pld) {
        memcpy(data + 1, pld, payloadLen());
    }

    /**
     * Set this frame's sequence number.
     */
    void setSequence (unsigned char seq) {
        data[0] = seq;
    }

private:
    static const size_t PAYLOADLEN = 50;

    unsigned char data[PAYLOADLEN + 1];
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
     * Send a buffer over a "connected" UDP socket.
     */
    void send (const char *buf, size_t buflen) const;

    /**
     * Send a buffer over a UDP socket to an arbitrary address.
     */
    void send (const Address& addr, const char *s, size_t buflen) const;

    /**
     * Block until a packet is received, or a timer expires. Return true if a
     * packet has been successfully received, false if the timer expired.
     * buflen is an in/out parameter--it should initially be set to the length
     * of the buffer pointed to by buf; it will later be set to the actual
     * number of bytes received.
     */
    bool timedRecvFrom (Address& addr, char *buf, size_t& buflen, int microseconds) const;

    void recvFrom (Address& addr, char *buf, size_t& buflen) const;

private:
    int fd;
    bool connected;
};

#endif
