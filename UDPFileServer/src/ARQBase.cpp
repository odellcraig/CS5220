/*
 * ARQBase.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#include "ARQBase.h"

ARQBase::ARQBase(UDPSocket &iSocket) :
		mSocket(iSocket), mDestinationAddress(""), mDestinationPort(0) {
}

ARQBase::ARQBase(UDPSocket& iSocket, std::string &iDestinationAddress, uint16_t iDestinationPort) :
		mSocket(iSocket),
		mDestinationAddress(iDestinationAddress),
		mDestinationPort(iDestinationPort) {
}

ARQBase::~ARQBase() {
}

