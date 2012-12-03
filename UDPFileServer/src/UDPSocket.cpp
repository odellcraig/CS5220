/*
 * UDPSocket.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#include "UDPSocket.h"

// ***************************************************************************
//
// DESCRIPTION  UDPSocket - UDP socket connection
//
//
// ***************************************************************************

#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <stdint.h>

#include <iostream>

using namespace std;

#define DEBUG_INFO

// ***************************************************************************
UDPSocket::UDPSocket(): sockfd(-1), mDropPercentage(0) {
}

UDPSocket::UDPSocket(uint32_t iDropPercentage): sockfd(-1), mDropPercentage(iDropPercentage) {
	if(mDropPercentage) {
		cout << "Dropping " << mDropPercentage << " percent of packets.\n";
	}
	srand ( time(NULL) );
}


// ***************************************************************************
UDPSocket::~UDPSocket() {
	// Close the socket if it is still open.
	if (sockfd >= 0) {
		// Try to close the socket
		if (!close()) {
			cerr << "UDPSocket::~UDPSocket()  close() failed, sockfd="
					<< sockfd << '\n';
		}
	}
}

// ***************************************************************************
bool UDPSocket::createAndBind(unsigned short udpPort,
		const std::string* addressString/* = NULL*/) {
	return createSocket() && setSocketOptions() && bind(udpPort, addressString);
}

// ***************************************************************************
bool UDPSocket::createSocket() {
	// Make sure the socket does not already exist
	if (sockfd >= 0) {
		cerr << "UDPSocket::createSocket()  socket already exists, sockfd="
				<< sockfd;
		return false;
	}

	// Create a UDP socket
	sockfd = (int) ::socket(socketFamily(), SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		cerr << "UDPSocket::createSocket()  ::socket() failed, sockfd="
				<< sockfd;
		sockfd = -1;
		return false;
	}


	return true;
}

// ***************************************************************************
bool UDPSocket::bind(unsigned short udpPort, const std::string* addressString) {
	// Make sure the socket does already exists
	if (sockfd < 0) {
		cerr << "UDPSocket::bind()  socket does not exist, sockfd=" << sockfd;
		return false;
	}

	// Create the sock addr structure
	int addrSize;
	sockaddr *addr = createSockAddr(udpPort, addressString, addrSize);
	if (addr == NULL) {
		close();
		return false;
	}

	// Bind the socket to UDP port
	int bindRtn = ::bind(sockfd, addr, addrSize);
	if (bindRtn < 0) {
		cerr << "UDPSocket::bind()  ::bind() failed, sockfd=" << sockfd;
		close();
		delete addr;
		return false;
	}

	delete addr;
	return true;
}

// ***************************************************************************
int UDPSocket::recvFrom(char* buf, int bufLen, std::string* srcAddrString,
		unsigned short* srcPort, int flags) {
	// Make sure the socket exists
	if (sockfd < 0) {
		cerr << "UDPSocket::recvFrom()  socket does not exist, sockfd="
				<< sockfd;
		return -1;
	}

	// Create the sock addr structure
	int addrSize;
	sockaddr *srcAddr = createSockAddr(addrSize);

	int recvLen = ::recvfrom(sockfd, buf, bufLen, flags, srcAddr,
			(socklen_t*) &addrSize);
	if (recvLen > 0) {
		if (srcAddrString != NULL) {
			getName(srcAddr, *srcAddrString);
		}
		if (srcPort != NULL) {
			getPort(srcAddr, *srcPort);
		}
	}

	delete srcAddr;
	return recvLen;
}

bool UDPSocket::hasData(int timeOut) {
	fd_set socketReadSet;
	FD_ZERO(&socketReadSet);
	FD_SET(sockfd, &socketReadSet);
	struct timeval tv;
	if (timeOut) {
		tv.tv_sec = timeOut / 1000;
		tv.tv_usec = (timeOut % 1000) * 1000;
	} else {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	} // if
	if (select(sockfd + 1, &socketReadSet, 0, 0, &tv) == -1) {
		return false;
	} // if
	return FD_ISSET(sockfd,&socketReadSet) != 0;
}

// ***************************************************************************
int UDPSocket::sendTo(const char* buf, int bufLen,
		const std::string& destAddressString, unsigned short destPort,
		int flags) {
	// Make sure the socket exists
	if (sockfd < 0) {
		cerr << "UDPSocket::sendTo()  socket does not exist, sockfd=" << sockfd;
		return -1;
	}

	// Create the sock addr structure
	int addrSize;
	sockaddr *addr = createSockAddr(destPort, &destAddressString, addrSize);
	if (addr == NULL) {
		close();
		return false;
	}

	int sendRtn = 0;
	if(!isRandomDrop()) {
		sendRtn = ::sendto(sockfd, (char*) buf, bufLen, flags, addr, addrSize);
	}
	else {

#ifdef DEBUG_INFO
		cout << "Random Drop!\n";
#endif
		sendRtn = bufLen;
	}
	delete addr;
	return sendRtn;
}

// ***************************************************************************
bool UDPSocket::close() {
	// Make sure the socket exists
	if (sockfd < 0) {
		cerr << "UDPSocket::close()  socket does not exist, sockfd=" << sockfd << '\n';
		return false;
	}

	int closeRtn;
	closeRtn = ::close(sockfd);
	if (closeRtn < 0) {
		cerr << "UDPSocket::closeNative()  ::close() failed, sockfd=" << sockfd << '\n';
		cerr << "Errno: " << errno << '\n';
		return false;
	}

	//Mark the socket as closed
	sockfd = -1;
	return true;
}

// ***************************************************************************
int UDPSocket::socketFamily() {
	return AF_INET;
}

// ***************************************************************************
sockaddr*
UDPSocket::createSockAddr(unsigned short udpPort,
		const std::string* addressString, int& addrSize) {
	sockaddr_in *addr = new sockaddr_in;

	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	if (addressString == NULL) {
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		// Get the internet address
		addr->sin_addr.s_addr = inet_addr((char*) addressString->c_str());
		if (addr->sin_addr.s_addr == (unsigned long) (-1)) {
			cerr << "UDPSocket::createSockAddr()  ::inet_addr() failed - "
					<< *addressString;
			delete addr;
			addrSize = 0;
			return NULL;
		}

	}
	addr->sin_port = htons(udpPort);

	addrSize = sizeof(sockaddr_in);
	return (sockaddr*) addr;
}

// ***************************************************************************
sockaddr*
UDPSocket::createSockAddr(int& addrSize) {
	sockaddr_in *addr = new sockaddr_in;

	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;

	addrSize = sizeof(sockaddr_in);
	return (sockaddr*) addr;
}

// ***************************************************************************
bool UDPSocket::getName(sockaddr *addr, std::string& addressString) {
	const char *name;

	char addrstr[INET_ADDRSTRLEN];
	name = ::inet_ntop(AF_INET, &((sockaddr_in*) addr)->sin_addr, addrstr,
			INET_ADDRSTRLEN);

	if (name == NULL) {
		addressString = "unknown";

		cerr << "UDPSocket::getName()  ::inet_ntop() failed - "
				<< addressString;
		return false;
	}

	addressString = name;
	return true;
}

// ***************************************************************************
bool UDPSocket::getPort(sockaddr *addr, unsigned short& srcPort) {
	srcPort = ntohs(((sockaddr_in*) addr)->sin_port);
	return true;
}

// ***************************************************************************
bool UDPSocket::setSocketOptions() {
	// This class doesn't need to do anything here but derived classes might.
	return true;
}

bool UDPSocket::isRandomDrop() {


	if(mDropPercentage == 0) {
		return false;
	}


	unsigned int randomNumber = (rand()%100) + 1; //Random number between 1 and 100


	if(randomNumber <= mDropPercentage) {
		return true;
	}
	return false;
}

