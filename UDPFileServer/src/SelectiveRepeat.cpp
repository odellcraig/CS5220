/*
 * SelectiveRepeat.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#include "SelectiveRepeat.h"
#include "Util.h"
#include "GuardMutex.h"

#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <pthread.h>

using namespace std;


namespace {
	const int HEADERSIZE = 10;
	const int RETX_LIMIT = 10;

	//Sleep times
	const int SEND_SLEEP_US = 1000*100;    //100ms
	const int MONITOR_SLEEP_US = 1000*100; //100ms
	const int DATA_WAIT_SLEEP_US = 1000*10;

	//Timeout
	const int RETX_TIMEOUT_MS = 1000*5; // 1 second

	const int DEFAULT_WINDOW_SIZE = 256;


	//Threads for Sending, Receiving, and Monitoring
	pthread_t senderThread, receiverThread, monitorThread;

	pthread_mutex_t mutexSendBuffer = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutexReceiveBuffer = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutexRecieveStream = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutexStateEdit = PTHREAD_MUTEX_INITIALIZER;

	deque<SelectiveRepeat::Segment> sendBuffer;
	deque<SelectiveRepeat::Segment> receiveBuffer;
	deque<unsigned char> receiveStream;

	void *SenderThreadFunction(void *sr);
	void *ReceiverThreadFunction(void *sr);
	void *MonitorThreadFunction(void *sr);
}


/****************************************************************/
SelectiveRepeat::SelectiveRepeat(UDPSocket &iSocket) :
		ARQBase(iSocket),
		mCurrentAck(0),
		mCurrentSeq(0),
		mLastReceivedAck(0),
		mFarWindowsize(256)
{
	startThreads();
}


/****************************************************************/
SelectiveRepeat::SelectiveRepeat(UDPSocket& iSocket, std::string& iDestinationAddress,
		uint16_t iDestinationPort) :
		ARQBase(iSocket, iDestinationAddress, iDestinationPort),
		mCurrentAck(0),
		mCurrentSeq(0),
		mLastReceivedAck(0),
		mFarWindowsize(256){

	cout << "SelectiveRepeat::SelectiveRepeat - Original servername: " << iDestinationAddress << endl;
	struct hostent *h;						/* info about server */
	h = gethostbyname(iDestinationAddress.c_str());		/* look up host's IP address */
	if (!h) {
		throw string("Error - gethostbyname() failed. Aborting.");
	}
	sockaddr_in m_addr;
	memcpy(&m_addr.sin_addr.s_addr, h->h_addr, h->h_length);
	mDestinationAddress = inet_ntoa(m_addr.sin_addr);

	cout << "SelectiveRepeat::SelectiveRepeat - IP Address of server: " << mDestinationAddress << endl;

	startThreads();
}




/****************************************************************/
SelectiveRepeat::~SelectiveRepeat() {
	//Turn off everything
	pthread_cancel(senderThread);
	pthread_cancel(receiverThread);
	pthread_cancel(monitorThread);
}


/****************************************************************/
void SelectiveRepeat::sendString(std::string sendStr) {
	deque<unsigned char> buffer(sendStr.begin(), sendStr.end());
	buffer.push_back('\0'); //Add so far side knows when to stop
	sendData(buffer);
}

/****************************************************************/
void SelectiveRepeat::sendInt(uint32_t i) {
	uint32_t sendInt = htonl(i);
	unsigned char bytes[4];
	memset(bytes, 0, sizeof(bytes));
	memcpy(bytes, &sendInt, sizeof (sendInt));
	deque<unsigned char> buffer(&bytes[0], &bytes[4]);
	sendData(buffer);
}

/****************************************************************/
void SelectiveRepeat::sendData(deque<unsigned char>& buffer) {
	{ //Guard sendBuffer
		GuardMutex G(&mutexSendBuffer);
		while(buffer.size()) {
			SelectiveRepeat::Segment seg;

			//Either going to create a full packet, or consume the whole buffer
			size_t bytesTaken = min(buffer.size(), (size_t)mSocket.getMaxSegmentSize(HEADERSIZE));

			// Put the data in the segment
			seg.data.insert(seg.data.begin(), buffer.begin(), buffer.begin()+bytesTaken);

			// Pull the data out of the stream
			buffer.erase(buffer.begin(), buffer.begin()+bytesTaken);

			seg.needsTransmit = true;
			seg.header.seqNumber = mCurrentSeq++; //Should be only place that updates mCurrentSeq
			//Note: sendTime,ackNumber,windowSize set when segment is sent.

			sendBuffer.push_back(seg);
		}
	} //Guard sendBuffer
}



/****************************************************************/
string SelectiveRepeat::recvString() {
	stringstream ss;

	while(true) { //Keep going until we get a null character
		{ //Guard receiveStream
			GuardMutex G(&mutexRecieveStream);
			if(receiveStream.size()) {
				while(receiveStream.size()) {
					ss << receiveStream.front();
					if(receiveStream.front() == '\0') {
						receiveStream.pop_front();
						return ss.str();
					}
					receiveStream.pop_front();
				}
			}
		} //Guard receiveStream

		//Let receive engine receive more data
		usleep(DATA_WAIT_SLEEP_US);
	}

	//Unreachable - get rid of warning
	return "";
}


/****************************************************************/
uint32_t SelectiveRepeat::recvInt() {
	deque<unsigned char> buffer;
	recvData(buffer, 4);

	unsigned char bytes[4];
	for(int i = 0; i < 4; ++i) {
		bytes[i] = buffer[i];
	}
	uint32_t returnInt;
	memcpy(&returnInt, bytes, 4);
	returnInt = ntohl(returnInt);
	return returnInt;
}


/****************************************************************/
void SelectiveRepeat::recvData(deque<unsigned char>& buffer,
		unsigned int size) {

	// Clear any data that might be hanging around
	if(buffer.size()) { buffer.erase(buffer.begin(), buffer.end()); }

	// Receive exact number of bytes
	while(size) {
		{ //Guard receiveStream
			GuardMutex G(&mutexRecieveStream);
			if(receiveStream.size()) {
				size_t byteCount = min(receiveStream.size(), (size_t)size);
				for(size_t i = 0; i < byteCount; ++i) {
					buffer.push_back(receiveStream.front());
					receiveStream.pop_front();
					size--;
				}
			}

		} //Guard receiveStream
		//Let receive engine receive more data
		usleep(DATA_WAIT_SLEEP_US);
	}
}


/****************************************************************/
void SelectiveRepeat::startThreads() {
	cout << "SelectiveRepeat::startThreads - start\n";

	if(pthread_create(&senderThread, NULL, &SenderThreadFunction, (void *)this))
		throw string("Error - creating sender thread.");

	if(pthread_create(&receiverThread, NULL, &ReceiverThreadFunction, (void *)this))
			throw string("Error - creating receiver thread.");

	if(pthread_create(&monitorThread, NULL, &MonitorThreadFunction, (void *)this))
			throw string("Error - creating monitor thread.");

	cout << "SelectiveRepeat::startThreads - finish\n";
}


/****************************************************************/
void SelectiveRepeat::sendSegment(Segment& seg) {

	// Set the current ack for every outgoing packet
	seg.header.ackNumber  = mCurrentAck;

	//Get the receive buffer size
	int receiveBufferSize = 0;
	{
		GuardMutex G(&mutexReceiveBuffer);
		receiveBufferSize = receiveBuffer.size();
	}

	// Set our window size for every outgoing packet
	seg.header.windowSize = DEFAULT_WINDOW_SIZE-receiveBufferSize;


	char sendBytes[seg.data.size() + HEADERSIZE];
	seg.getHeaderIntoArray(sendBytes);
	for(size_t i = 0; i < seg.data.size(); ++i) {
		sendBytes[i+HEADERSIZE] = seg.data[i];
	}

	cout << "sendSegment():\n";
	cout << "  Seq = " << seg.header.seqNumber << endl;
	cout << "  Ack = " << seg.header.ackNumber << endl;
	cout << "  Win = " << seg.header.windowSize << endl;
	cout << "  Datalen = " << seg.data.size() << endl;
	//Util::dump(seg.data);

	mSocket.sendTo(sendBytes, sizeof(sendBytes),mDestinationAddress, mDestinationPort);

	//sleep(1);
}

void SelectiveRepeat::sendAck() {

	SelectiveRepeat::Segment seg;

	seg.needsTransmit = true;
	seg.header.seqNumber = mCurrentSeq; //Don't increment seq for ctl packets



	//Note: sendTime,ackNumber,windowSize set when segment is sent.
	{ // Guard sendBuffer

		cout << "sendAck() - adding Ack to front of queue. Size was = " << sendBuffer.size() << endl;
		GuardMutex G(&mutexSendBuffer);
		sendBuffer.push_front(seg);
		cout << "sendAck() - adding Ack to front of queue. Size now = " << sendBuffer.size() << endl;

	} // Guard sendBuffer
}


/****************************************************************/
void SelectiveRepeat::processDataSegment(Segment& seg) {
	{ // Guard receiveBuffer, receiveStream
		GuardMutex G1(&mutexReceiveBuffer);
		GuardMutex G2(&mutexRecieveStream);

		// First, add this segment to the receiveBuffer in order if we have not yet received this segment
		deque<SelectiveRepeat::Segment>::iterator it = receiveBuffer.begin();
		while(it != receiveBuffer.end() && it->header.seqNumber < seg.header.seqNumber) {
			++it;
		}
		//Don't add duplicates, so make sure we're at end or next is strictly greater than the one we're adding
		if(it == receiveBuffer.end() || it->header.seqNumber > seg.header.seqNumber) {
			receiveBuffer.insert(it, seg);
		}


		// Second, scan through to find our next expected segment and set mCurrentAck
		for(it = receiveBuffer.begin(); it != receiveBuffer.end(); ++it) {
			// Starting from the beginning, if this is our expected data, then check for more
			// continue until we've reached a data gap or the end.
			if(it->header.seqNumber == mCurrentAck){
				mCurrentAck++; //Note: this should be the only place this gets edited
			}
			else {
				break;
			}
		}

		// Now, if we had data that was ack'd push those data packets upstream
		if(it != receiveBuffer.begin()) {
			deque<SelectiveRepeat::Segment>::iterator donePackets;
			for(donePackets = receiveBuffer.begin(); donePackets != it; ++donePackets) {
				//Add data to receiveStream
				receiveStream.insert(receiveStream.end(), donePackets->data.begin(), donePackets->data.end());
			}
			receiveBuffer.erase(receiveBuffer.begin(), it);
		}

	} // Guard receiveBuffer, receiveStream

	// Third, send an ack for the next segment we're expecting
	// Always send an ack when we receive a data packet because even if it's a dup data or out of order,
	// we want to make sure the far side knows what seq number we're expecting
	sendAck();
}



/****************************************************************/
void SelectiveRepeat::processSegmentHeader(Segment& seg) {
	//For each data packet in the sendBuffer, if now ack'd, remove it
	{ // Guard sendBuffer, StateEdit
		GuardMutex G1(&mutexSendBuffer);
		GuardMutex G2(&mutexStateEdit);

		// 1. Remove any data now acked
		deque<SelectiveRepeat::Segment>::iterator it;
		// Find the greatest position where all data is now acked
		for(it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
			if(seg.header.ackNumber <= it->header.seqNumber)
				break;
		}
		//If this acked some of our data, remove it from the buffer
		if(it != sendBuffer.begin()) {
			sendBuffer.erase(sendBuffer.begin(), it);
		}

		// 2. Check for a duplicate ack (Non-data packet with same ack as last packet)
		if(seg.data.size() == 0 && seg.header.ackNumber == mLastReceivedAck) {
			//find the data packet that the far side is missing and mark for retransmit
			deque<SelectiveRepeat::Segment>::iterator it;
			for(it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
				if(seg.header.ackNumber == it->header.seqNumber) {
					it->needsRetransmit = true;
				}
			}
		}

		// 3. Update the state
		mLastReceivedAck = seg.header.ackNumber;
		mFarWindowsize   = seg.header.windowSize;

	} // Guard sendBuffer, StateEdit

}




namespace {

/****************************************************************/
void *SenderThreadFunction(void *sr){
	SelectiveRepeat *me = (SelectiveRepeat*)sr;

	cout << "SelectiveRepeat::SenderThreadFunction - thread started\n";

	while(true) {
		bool sentSegment = false;

		{  // Guard sendBuffer
			GuardMutex Guard(&mutexSendBuffer);

			deque<SelectiveRepeat::Segment>::iterator it;
			for(it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
				if(it->needsTransmit || it->needsRetransmit) {

					//If we have too much outstanding data, don't send this data packet, but keep looking for any ctl packets
					if(it->data.size() && (me->mFarWindowsize - (sendBuffer.size()-1)) <= 0) { continue; } // -1 is so we don't count this packet that's now in the buffer

					//Send the segment
					me->sendSegment(*it);

					// If it was a data segment, update the state
					if(it->data.size()) {
						//Update the segment flags
						it->sendTime = Util::getCurrentTimeMs();
						it->needsTransmit = false;
						it->needsRetransmit = false;
					}
					// Otherwise, don't keep it
					else {
						//Note: this invalidates all iterators, so break out and loop again
						sendBuffer.erase(it);
						break;
					}

					sentSegment = true;
				}
			}
		} // Guard sendBuffer

		//If we didn't have data, then chill for a bit
		usleep(SEND_SLEEP_US);
	}
	return NULL;
}

/****************************************************************/
void *ReceiverThreadFunction(void *sr){
	SelectiveRepeat *me = (SelectiveRepeat*)sr;

	cout << "SelectiveRepeat::ReceiverThreadFunction - thread started\n";

	//Create space to drop data
	char receiveArray[me->mSocket.getMaxSegmentSize(HEADERSIZE) + HEADERSIZE];
	memset(receiveArray, 0, sizeof(receiveArray));
	int recvBytes = 0;

	// Do this forever
	while(true) {

		//Only go through the trouble if we have data
		if(!me->mSocket.hasData()) { continue; }

		//If this is the first receive, then set the far side address/port
		if(me->mDestinationAddress == "" || me->mDestinationPort == 0) {
			recvBytes = me->mSocket.recvFrom(receiveArray, sizeof(receiveArray), &me->mDestinationAddress, &me->mDestinationPort);
		}
		else {
			recvBytes = me->mSocket.recvFrom(receiveArray, sizeof(receiveArray));
		}

		//Create a workable object
		SelectiveRepeat::Segment seg(receiveArray, recvBytes);

		cout << "\nreceiveSegment():\n";
		cout << "  Seq = " << seg.header.seqNumber << endl;
		cout << "  Ack = " << seg.header.ackNumber << endl;
		cout << "  Win = " << seg.header.windowSize << endl;
		cout << "  Datalen = " << seg.data.size() << endl;
		//Util::dump(seg.data);

		if(true) // TODO: only receive if within our sliding window
		{
			me->processSegmentHeader(seg);

			//If there's data, put in receive buffer
			if(seg.data.size()) {
				me->processDataSegment(seg);
			}
		}
	}

	return NULL;
}

/****************************************************************/
void *MonitorThreadFunction(void *sr){

	cout << "SelectiveRepeat::MonitorThreadFunction - thread started\n";

	// Do this forever
	while(true) {

		//Chill for a bit
		usleep(MONITOR_SLEEP_US);

		{ //Guard sendBuffer
			GuardMutex Guard(&mutexSendBuffer);
			//See if any packets need retransmitting
			deque<SelectiveRepeat::Segment>::iterator it;
			for(it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
				if((Util::getCurrentTimeMs()-it->sendTime) > RETX_TIMEOUT_MS) {
					it->needsRetransmit = true;
				}
			}
		} //Guard sendBuffer
	}
	return NULL;
}
}







