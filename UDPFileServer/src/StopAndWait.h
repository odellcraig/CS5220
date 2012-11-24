/*
 * StopAndWait.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef STOPANDWAIT_H_
#define STOPANDWAIT_H_

#include "ARQBase.h"

#include <deque>
#include <stdint.h>

class StopAndWait: public ARQBase {
public:
	StopAndWait(UDPSocket &iSocket);
	StopAndWait(UDPSocket &iSocket, std::string &iDestinationAddress, uint16_t iDestinationPort);
	virtual ~StopAndWait();

	//Helper methods for easy transfer of data, strings, and ints
	virtual void sendString(std::string sendStr);
	virtual void sendInt(uint32_t i);
	virtual void sendData(std::deque<unsigned char> &);
	virtual bool sendDatagramWaitForAck(char *buf, int length);

	/**
	 * Remove data from deque and put into buf. Return the number of bytes written into buf
	 */
	virtual int consumeData(char *buf, std::deque<unsigned char> &buffer, unsigned int maxBufSize);


	virtual void addHeaderToFront(std::deque<unsigned char> &buffer);
	virtual void consumeHeaderSendAck(uint8_t seq, uint8_t ack);

	/**
	 * Sends an independent ack for a received data frame
	 */
	virtual void sendAck();

	/**
	 * Returns true if the received independent ack has the expected ack number
	 */
	virtual bool recvAck();


	//Helper methods for easy reception of data, strings, ints
	virtual std::string recvString();
	virtual uint32_t	recvInt();

	/**
	 * Passing a size of zero will receive a single datagram
	 */
	virtual void 		recvData(std::deque<unsigned char> &, unsigned int size);

protected:
	uint8_t mSequenceNumber;		// 0 or 1 - starting with 0
	uint8_t mAckNumber;				// 0 or 1 - ack number of 0 means expecting packet with seq 0
	uint8_t mLastReceivedAckNumber; // for dup-ack detection

};

#endif /* STOPANDWAIT_H_ */
