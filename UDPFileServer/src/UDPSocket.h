/*
 * FnetSocketDatagram.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */


// ***************************************************************************
//
// This class can be used as either a UDP server, or a UDP client.  The constructor
// provides a generic socket that is made a server or client by the functions that
// are used.
//
// The "std::string addressString" arguments in the following functions are ASCII
// IPv4 addresses in dotted decimal form.  Example: "129.196.196.12"
//
// ***************************************************************************

#ifndef UDPSOCKET_H_
#define UDPSOCKET_H_

#include <string>

struct sockaddr;


class UDPSocket
{
public:
    UDPSocket();
    virtual ~UDPSocket();

    unsigned int getMaxSegmentSize() {return 1472-2;}

    // A UDP server socket is created with this function.  The code would
    // typically call and block on recvFrom() until packets arrive.
    bool createAndBind(unsigned short udpPort, const std::string* addressString = NULL);

    bool createSocket();

    bool bind(unsigned short udpPort, const std::string* addressString = NULL);

    // Block until a packet arrives on this bound socket.  The srcAddressString
    // and srcPort arguments are in/out parameters.  The source address and
    // source port number will be written through these arguments when a packet
    // arrives.
    // The buf argument should be large enough to hold the complete payload of
    // a UDP packet.  e.g. 1500
    int recvFrom(char* buf, int bufLen,
                 std::string* srcAddressString = NULL, unsigned short* srcPort = NULL,
                 int flags = 0);

    // Send the "buf" data to the given address and port.
    int sendTo(const char* buf, int bufLen,
               const std::string& destAddressString, unsigned short destPort,
               int flags = 0);

    bool hasData(int timeOut); //milliseconds

    bool close();

protected:
    // Virtual functions to allow IPv6 implementation.
    virtual int       socketFamily();
    virtual sockaddr* createSockAddr(unsigned short tcpPort, const std::string* addressString,
                                     int& addrSize);
    virtual sockaddr* createSockAddr(int& addrSize);
    virtual bool      getName(sockaddr *addr, std::string& addressString);
    virtual bool      getPort(sockaddr *addr, unsigned short& srcPort);

    // This function will be called by the createAndBind() function after the
    // socket has been created but before the bind().
    virtual bool setSocketOptions();


protected:
    int sockfd;
};


#endif /* UDPSOCKET_H_ */
