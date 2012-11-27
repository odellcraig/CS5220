/*
 * ARQBase.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef ARQBASE_H_
#define ARQBASE_H_

#include "UDPSocket.h"

#include <string>
#include <deque>
#include <stdint.h>

class ARQBase {
public:


	enum ARQType {
		ARQTypeStopAndWait     = 1,
		ARQTypeGoBackN         = 2,
		ARQTypeSelectiveRepeat = 3
	};

	ARQBase(UDPSocket &iSocket);
	ARQBase(UDPSocket &iSocket, std::string &iDestinationAddress, uint16_t iDestinationPort);
	virtual ~ARQBase();


	//Helper methods for easy transfer of data, strings, and ints
	virtual void sendString(std::string sendStr) = 0;
	virtual void sendInt(uint32_t i) = 0;
	virtual void sendData(std::deque<unsigned char> &) = 0;

	//Helper methods for easy reception of data, strings, ints
	virtual std::string recvString() = 0;
	virtual uint32_t			recvInt() = 0;
	virtual void 		recvData(std::deque<unsigned char> &, unsigned int size) = 0;



	UDPSocket &mSocket;
	std::string mDestinationAddress;
	uint16_t mDestinationPort;

};

#endif /* ARQBASE_H_ */
