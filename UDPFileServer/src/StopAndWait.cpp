/*
 * StopAndWait.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#include "StopAndWait.h"
#include "Util.h"

#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>

using namespace std;


namespace {
	const int TIMEOUT = 1000; // 1000ms timeout
	const int HEADERSIZE = 2;
	const int RETX_LIMIT = 10;
}

StopAndWait::StopAndWait(UDPSocket &iSocket) :
		ARQBase(iSocket),
		mSequenceNumber(0),
		mAckNumber(0),
		mLastReceivedAckNumber(-1)
{}


StopAndWait::StopAndWait(UDPSocket& iSocket, std::string& iDestinationAddress,
		uint16_t iDestinationPort) :
		ARQBase(iSocket, iDestinationAddress, iDestinationPort),
		mSequenceNumber(0),
		mAckNumber(0),
		mLastReceivedAckNumber(-1){

	cout << "StopAndWait::StopAndWait - Original servername: " << iDestinationAddress << endl;
	struct hostent *h;						/* info about server */
	h = gethostbyname(iDestinationAddress.c_str());		/* look up host's IP address */
	if (!h) {
		throw string("Error - gethostbyname() failed. Aborting.");
	}
	sockaddr_in m_addr;
	memcpy(&m_addr.sin_addr.s_addr, h->h_addr, h->h_length);
	mDestinationAddress = inet_ntoa(m_addr.sin_addr);

	cout << "StopAndWait::StopAndWait - IP Address of server: " << mDestinationAddress << endl;
}

StopAndWait::~StopAndWait() {}

void StopAndWait::sendString(std::string sendStr) {
	if(sendStr.length() > mSocket.getMaxSegmentSize()) {
		throw string("Error - attempting to send string of size greater than internally defined max UDP length.");
	}

	deque<unsigned char> buffer(sendStr.begin(), sendStr.end());
	sendData(buffer);
}

void StopAndWait::sendInt(uint32_t i) {
	uint32_t sendInt = htonl(i);
	unsigned char bytes[4];
	memset(bytes, 0, sizeof(bytes));
	memcpy(bytes, &sendInt, sizeof (sendInt));
	deque<unsigned char> buffer(&bytes[0], &bytes[4]);
	sendData(buffer);
}

void StopAndWait::sendData(deque<unsigned char>& buffer) {

	char buf[mSocket.getMaxSegmentSize()];
	memset(buf, 0, mSocket.getMaxSegmentSize());

	while(buffer.size()) {

		addHeaderToFront(buffer);												 // Add the header
		mSequenceNumber = (mSequenceNumber+1)%2;								 // Increase seq number for packets after this one
		int bytesCopied = consumeData(buf, buffer, mSocket.getMaxSegmentSize()); // Move that packet into buf

		//Retx Loop Waits for Ack
		bool success;
		int retxCount = 0;
		do {
			success = sendDatagramWaitForAck(buf, bytesCopied);
			if(!success) cerr << "Retransmit!\n";
			retxCount++;

		//Wait for either ack, dup-ack, or timeout
		}while(!success && retxCount < RETX_LIMIT);

		if(retxCount >= RETX_LIMIT) {
			cerr << "Retransmission Limit Reached. Aborting.";
			return;
		}

	}
}

bool StopAndWait::sendDatagramWaitForAck(char *buf, int length)
{
	cout << "Sending Datagram of length: " << length << endl;

	//Send the packet
	mSocket.sendTo(buf, length, mDestinationAddress, mDestinationPort);
	uint32_t sendTime = Util::getCurrentTimeMs();


	//Retransmit if dup-ack or timeout
	do {
		//If we have data
		if(mSocket.hasData(10)) {
			//If we received the correct ack, return
			//If it was a duplicate ack, recv ack will return false
			if(recvAck()) {return true; }
			else {return false;}
		}



	}while((Util::getCurrentTimeMs() - sendTime) < TIMEOUT);

	cerr << "Timeout!" << endl;

	return false;
}



void StopAndWait::addHeaderToFront(std::deque<unsigned char>& buffer) {
	buffer.push_front(mAckNumber);
	buffer.push_front(mSequenceNumber);
}


int StopAndWait::consumeData(char* buf, deque<unsigned char>& buffer,
		unsigned int maxBufSize) {

	//Clear out the buffer
	memset(buf, 0, maxBufSize);

	//If we have more data than the buffer can hold, then put as much as we can
	if(buffer.size() > maxBufSize) {
		for(size_t i = 0; i < maxBufSize; ++i){
			buf[i] = buffer[i];
		}
		buffer.erase(buffer.begin(), buffer.begin()+maxBufSize);
		return maxBufSize;
	}

	//Otherwise, consume the rest
	for(size_t i = 0; i < buffer.size(); ++i) {
		buf[i] = buffer[i];
	}
	int bytesWritten = buffer.size();
	buffer.erase(buffer.begin(), buffer.end());
	return bytesWritten;
}



string StopAndWait::recvString() {
	stringstream ss;
	deque<unsigned char> buffer;
	recvData(buffer, 0); //Passing in a size of 0 receives a single datagram
	for(size_t i = 0; i < buffer.size(); ++i) {
		ss << buffer[i];
	}
	return ss.str();
}

uint32_t StopAndWait::recvInt() {
	deque<unsigned char> buffer;
	recvData(buffer, 0); //Passing in a size of 0 receives a single datagram
	unsigned char bytes[4];
	for(int i = 0; i < 4; ++i) {
		bytes[i] = buffer[i];
	}
	uint32_t returnInt;
	memcpy(&returnInt, bytes, 4);
	returnInt = ntohl(returnInt);
	return returnInt;
}





void StopAndWait::recvData(deque<unsigned char>& buffer,
		unsigned int size) {

	buffer.erase(buffer.begin(), buffer.end());
	char receiveBuffer[mSocket.getMaxSegmentSize()];
	memset(receiveBuffer, 0, mSocket.getMaxSegmentSize());

	//Receive a single datagram
	if(size == 0) {
		int recvBytes = 0;
		if(mDestinationAddress == "" || mDestinationPort == 0) {
			recvBytes = mSocket.recvFrom(receiveBuffer, mSocket.getMaxSegmentSize(), &mDestinationAddress, &mDestinationPort);
		}
		else {
			recvBytes = mSocket.recvFrom(receiveBuffer, mSocket.getMaxSegmentSize());
		}

		cout << "Received datagram of length: " << recvBytes << endl;

		buffer.insert(buffer.end(), &receiveBuffer[2], &receiveBuffer[recvBytes]);
		consumeHeaderSendAck(receiveBuffer[0], receiveBuffer[1]);
		return;
	}

	//Receive a specific amount of data
	while(size) {
		int recvBytes = 0;
		if(mDestinationAddress == "" || mDestinationPort == 0) {
			recvBytes = mSocket.recvFrom(receiveBuffer, mSocket.getMaxSegmentSize(), &mDestinationAddress, &mDestinationPort);
		}
		else {
			recvBytes = mSocket.recvFrom(receiveBuffer, mSocket.getMaxSegmentSize());
		}

		cout << "Received datagram of length: " << recvBytes << endl;

		//Only count datagram if it's our expected sequence number
		if(receiveBuffer[0] == mAckNumber){
			size -= (recvBytes-HEADERSIZE);
		}

		buffer.insert(buffer.end(), &receiveBuffer[2], &receiveBuffer[recvBytes]);
		consumeHeaderSendAck(receiveBuffer[0], receiveBuffer[1]);

		//Clear out receive buffer
		memset(receiveBuffer, 0, mSocket.getMaxSegmentSize());

		cout << "Bytes Remaining: " << size << endl;
	}

}

void StopAndWait::consumeHeaderSendAck(uint8_t seq, uint8_t ack) {

	mLastReceivedAckNumber = ack;

	// If this is the packet we expected, send ack increase ack number
	if(seq == mAckNumber) {
		mAckNumber = (mAckNumber+1)%2;
	}
	//Note: if this wasn't the expected seq, then the send ack will be a duplicate ack
	sendAck();
}



/**
 * Send independent ack for each data frame
 */
void StopAndWait::sendAck() {
	char buf[2];
	buf[0] = mSequenceNumber;
	buf[1] = mAckNumber;
	mSocket.sendTo(buf,2,mDestinationAddress, mDestinationPort);

	cout << "Send Ack: " << (int)mAckNumber << endl;
}
/**
 * Assumes that there is an independent ack for eack data frame
 */
bool StopAndWait::recvAck() {
	char buf[2];
	memset(buf, 0, 2);
	mSocket.recvFrom(buf,2);

	uint8_t receivedAck = buf[1];

	cout << "Recv Ack: " << (int)receivedAck << endl;

	if(receivedAck == mSequenceNumber) {
		mLastReceivedAckNumber = receivedAck;
		return true;
	}
	return false;
}

