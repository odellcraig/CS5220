/*
 * SelectiveRepeat.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef SELECTIVEREPEAT_H_
#define SELECTIVEREPEAT_H_


#include "ARQBase.h"

#include <deque>
#include <fstream>
#include <stdint.h>
#include <cstring>
#include <arpa/inet.h>

class SelectiveRepeat: public ARQBase {
public:

#pragma pack(push, 1) //exact fit, no padding
	class SegmentHeader {
		public:
			uint32_t seqNumber;
			uint32_t ackNumber;
			uint16_t windowSize;
	};
#pragma pack(pop) // back to whatever padding it was

	class Segment {
	public:
		Segment(char *bytes, size_t length):
			sendTime(0),
			needsTransmit(false),
			needsRetransmit(false),
			isLastLogicalSegment(false),
			retxCount(0) {
			setHeaderFromArray(bytes);

			//If data
			if(length > sizeof(header)) {
				for(size_t i = sizeof(header); i < length; ++i) {
					data.push_back(bytes[i]);
				}
			}
		}

		Segment():
			sendTime(0),
			needsTransmit(false),
			needsRetransmit(false),
			isLastLogicalSegment(false),
			retxCount(0) {
			memset(&header, 0, sizeof(header));
		}



		void getHeaderIntoArray(char *buf){
			SegmentHeader tempHeader;
			//Make sure we're network byte order
			tempHeader.seqNumber  = htonl(header.seqNumber);
			tempHeader.ackNumber  = htonl(header.ackNumber);
			tempHeader.windowSize = htons(header.windowSize);
			memcpy(buf, &tempHeader, sizeof(tempHeader));
		}

		void setHeaderFromArray(char *buf) {
			memcpy(&header, buf, sizeof(header));
			//Make sure we have correct byte order
			header.seqNumber  = ntohl(header.seqNumber);
			header.ackNumber  = ntohl(header.ackNumber);
			header.windowSize = ntohs(header.windowSize);
		}

		void setHeader(uint32_t seq, uint32_t ack, uint16_t window) {
			header.seqNumber = seq;
			header.ackNumber = ack;
			header.windowSize = window;
		}



		uint32_t sendTime;
		bool needsTransmit;
		bool needsRetransmit;
		bool isLastLogicalSegment;
		unsigned int retxCount;
		SegmentHeader header;
		std::deque<unsigned char> data;
	};



	SelectiveRepeat(UDPSocket &iSocket, std::ofstream &traceFile);
	SelectiveRepeat(UDPSocket &iSocket, std::string &iDestinationAddress, uint16_t iDestinationPort, std::ofstream &traceFile);
	virtual ~SelectiveRepeat();

	virtual void close();

	//Helper methods for easy transfer of data, strings, and ints

	/**
	 * Send \0 terminated string
	 */
	virtual void sendString(std::string sendStr);

	/**
	 * Send 32-bit integer
	 */
	virtual void sendInt(uint32_t i);

	/**
	 * Send the data described by buffer by breaking it up into segments for the sender thread to consume
	 */
	virtual void sendData(std::deque<unsigned char> &buffer);

	/**
	 * Sends an ack for the current ack number
	 */
	virtual void sendAck();

	/**
	 * Receive a null terminated string from the recvStream that is populated by the receiver thread
	 */
	virtual std::string recvString();

	/**
	 * Consume 4 bytes from the recvStream and convert to 32-bit integer
	 */
	virtual uint32_t	recvInt();

	/**
	 * Receive size bytes from recvStream that is populated by the receive stream with in-order data
	 */
	virtual void 		recvData(std::deque<unsigned char> &, unsigned int size);


	void startThreads();

	void sendSegment(Segment &seg);

	/**
	 * This will:
	 * 1. Send ack for the data segment
	 * 2. Update any state
	 * 3. Send any data up to the byte stream if in order
	 */
	void processDataSegment(Segment &seg);

	/**
	 * This will:
	 * 1. Remove any data from sendBuffer that is now acked
	 * 2. Check for duplicate acks - mark any packets for retx
	 * 3. Update any state related to the header (latest recv ack, our next ack, far window)
	 */
	void processSegmentHeader(Segment &seg);


	uint32_t mCurrentAck; 		//The current ack we send in our packets to the far side. Updated when we send an Ack
	uint32_t mCurrentSeq; 		//The current seq we send in our packets updated when we send data except on retransmit
	uint32_t mLastReceivedAck;  //The last received ack used to detect dup-acks. Updated when we receive a dgram after we check for dup ack
	uint16_t mFarWindowsize;	//The advertised window of the far side (dictates how much we can send). Updated when we receive a dgram
								// Note: our window is calculated by Default - receiveBuffer.size()
								// Note: send window is mFarWindowSize - sendBuffer.size()
	uint32_t mOutstandingSegments;  // Unacked Segments

	std::ofstream &mTraceFile;
};

#endif /* SELECTIVEREPEAT_H_ */
