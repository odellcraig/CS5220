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

#define DEBUG_INFO
#define DEBUG_MORE
#define DEBUG_MOST

namespace {
const int HEADERSIZE = 10;
const unsigned int RETX_LIMIT = 10;

//Sleep times
const int SEND_SLEEP_US = 1000 * 100; //100ms
const int MONITOR_SLEEP_US = 1000 * 100; //100ms
const int DATA_WAIT_SLEEP_US = 1000 * 10;

//Timeout
const int RETX_TIMEOUT_MS = 1000 * 2; // 2 seconds

const int DEFAULT_WINDOW_SIZE = 1024;

//Threads for Sending, Receiving, and Monitoring
pthread_t senderThread, receiverThread, monitorThread;

pthread_mutex_t mutexSendBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexReceiveBuffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexRecieveStream = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexStateEdit = PTHREAD_MUTEX_INITIALIZER;

// Used to signal that a logical piece of data has been completed and acked
pthread_mutex_t mutexLastLogicalSegmentAckedOrRetxLimit =
		PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condLastLogicalSegmentAckedOrRetxLimit = PTHREAD_COND_INITIALIZER;

bool finishSending = false;

deque<SelectiveRepeat::Segment> sendBuffer;
deque<SelectiveRepeat::Segment> receiveBuffer;
deque<unsigned char> receiveStream;

void *SenderThreadFunction(void *sr);
void *ReceiverThreadFunction(void *sr);
void *MonitorThreadFunction(void *sr);



std::ostream& operator<<(std::ostream& os, const SelectiveRepeat::Segment& obj)
	{
		os << "Seq = " << obj.header.seqNumber;
		os << " Ack = " << obj.header.ackNumber;
		os << " Retx = " << obj.retxCount;
		os << " Win = " << obj.header.windowSize;
		os << " Len = " << obj.data.size();
		return os;
	}

}

/****************************************************************/
SelectiveRepeat::SelectiveRepeat(UDPSocket &iSocket, ofstream &traceFile) :
		ARQBase(iSocket), mCurrentAck(0), mCurrentSeq(0), mLastReceivedAck(0), mFarWindowsize(
				256), mOutstandingSegments(0),
				mTraceFile(traceFile) {
	startThreads();
}

/****************************************************************/
SelectiveRepeat::SelectiveRepeat(UDPSocket& iSocket,
		std::string& iDestinationAddress, uint16_t iDestinationPort, ofstream &traceFile) :
		ARQBase(iSocket, iDestinationAddress, iDestinationPort), mCurrentAck(0), mCurrentSeq(
				0), mLastReceivedAck(0), mFarWindowsize(256), mOutstandingSegments(0),
				mTraceFile(traceFile) {

#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::SelectiveRepeat - Original servername: "
			<< iDestinationAddress << endl;
#endif

	struct hostent *h; /* info about server */
	h = gethostbyname(iDestinationAddress.c_str()); /* look up host's IP address */
	if (!h) {
		throw string("Error - gethostbyname() failed. Aborting.");
	}
	sockaddr_in m_addr;
	memcpy(&m_addr.sin_addr.s_addr, h->h_addr, h->h_length);
	mDestinationAddress = inet_ntoa(m_addr.sin_addr);

#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::SelectiveRepeat - IP Address of server: "
			<< mDestinationAddress << endl;
#endif

	startThreads();
}

/****************************************************************/
SelectiveRepeat::~SelectiveRepeat() {
	//Clean all the globals up
	finishSending = false;

	sendBuffer.erase(sendBuffer.begin(), sendBuffer.end());
	receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.end());
	receiveStream.erase(receiveStream.begin(), receiveStream.end());
}

void SelectiveRepeat::close() {
	finishSending = true;
	pthread_join(senderThread, NULL);

	//Turn off everything else
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
	memcpy(bytes, &sendInt, sizeof(sendInt));
	deque<unsigned char> buffer(&bytes[0], &bytes[4]);
	sendData(buffer);
}

/****************************************************************/
void SelectiveRepeat::sendData(deque<unsigned char>& buffer) {
	{ //Guard sendBuffer
		GuardMutex G(&mutexSendBuffer);
		while (buffer.size()) {
			SelectiveRepeat::Segment seg;

			//Either going to create a full packet, or consume the whole buffer
			size_t bytesTaken = min(buffer.size(),
					(size_t) mSocket.getMaxSegmentSize(HEADERSIZE));

			// Put the data in the segment
			seg.data.insert(seg.data.begin(), buffer.begin(),
					buffer.begin() + bytesTaken);

			// Pull the data out of the stream
			buffer.erase(buffer.begin(), buffer.begin() + bytesTaken);

			seg.needsTransmit = true;
			seg.header.seqNumber = mCurrentSeq++; //Should be only place that updates mCurrentSeq
			//Note: sendTime,windowSize set when segment is sent.

			// If we're going to wait for signal, then mark the last logical segment
			if (buffer.size() == 0) {
				seg.isLastLogicalSegment = true;
			}

			sendBuffer.push_back(seg);
		}
	} //Guard sendBuffer

	// block until either retx limit or all the data is ack'd
	{ // Guard on LastLogicalSegmentAckedOrRetxLimit
		GuardMutex g(&mutexLastLogicalSegmentAckedOrRetxLimit);

#ifdef DEBUG_MOST
		cout << "Waiting for signal that last logical segment was acked or retx limit reached.\n";
#endif

		//Hang out until the last logical segment is acked, or we reach a retx limit on a segment
		pthread_cond_wait(&condLastLogicalSegmentAckedOrRetxLimit,
				&mutexLastLogicalSegmentAckedOrRetxLimit);

	} // Guard on LastLogicalSegmentAckedOrRetxLimit

}

/****************************************************************/
string SelectiveRepeat::recvString() {
	stringstream ss;

	while (true) { //Keep going until we get a null character
		{ //Guard receiveStream
			GuardMutex G(&mutexRecieveStream);
			if (receiveStream.size()) {
				while (receiveStream.size()) {
					ss << receiveStream.front();
					if (receiveStream.front() == '\0') {
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
	for (int i = 0; i < 4; ++i) {
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
	if (buffer.size()) {
		buffer.erase(buffer.begin(), buffer.end());
	}

	// Receive exact number of bytes
	while (size) {
		{ //Guard receiveStream
			GuardMutex G(&mutexRecieveStream);
			if (receiveStream.size()) {
				size_t byteCount = min(receiveStream.size(), (size_t) size);
				for (size_t i = 0; i < byteCount; ++i) {
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
#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::startThreads - start\n";
#endif

	if (pthread_create(&senderThread, NULL, &SenderThreadFunction,
			(void *) this))
		throw string("Error - creating sender thread.");

	if (pthread_create(&receiverThread, NULL, &ReceiverThreadFunction,
			(void *) this))
		throw string("Error - creating receiver thread.");

	if (pthread_create(&monitorThread, NULL, &MonitorThreadFunction,
			(void *) this))
		throw string("Error - creating monitor thread.");

#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::startThreads - finish\n";
#endif
}

/****************************************************************/
void SelectiveRepeat::sendSegment(Segment& seg) {


	seg.header.ackNumber = mCurrentAck;

	//Get the receive buffer size
	int receiveBufferSize = 0;
	{
		GuardMutex G(&mutexReceiveBuffer);
		receiveBufferSize = receiveBuffer.size();
	}

	// Set our window size for every outgoing packet
	seg.header.windowSize = DEFAULT_WINDOW_SIZE - receiveBufferSize;

	char sendBytes[seg.data.size() + HEADERSIZE];
	seg.getHeaderIntoArray(sendBytes);
	for (size_t i = 0; i < seg.data.size(); ++i) {
		sendBytes[i + HEADERSIZE] = seg.data[i];
	}

	mTraceFile << seg << endl;

#ifdef DEBUG_MOST
	cout << "sendSegment():\n";
	cout << "  Seq = " << seg.header.seqNumber << endl;
	cout << "  Ack = " << seg.header.ackNumber << endl;
	cout << "  Win = " << seg.header.windowSize << endl;
	cout << "  Ret = " << seg.needsRetransmit << endl;
	cout << "  RC  = " << seg.retxCount << endl;
	cout << "  Datalen = " << seg.data.size() << endl;
	//Util::dump(seg.data);
#endif

	mSocket.sendTo(sendBytes, sizeof(sendBytes), mDestinationAddress,
			mDestinationPort);

	mOutstandingSegments++;

}

void SelectiveRepeat::sendAck() {


	bool ackUpdated = false;


	//Note: sendTime,ackNumber,windowSize set when segment is sent.
	{ // Guard sendBuffer
#ifdef DEBUG_MOST
		cout << "sendAck() - adding or updating Ack. Size was = "
				<< sendBuffer.size() << endl;
#endif
		GuardMutex G(&mutexSendBuffer);

		//First Check and see if there's an ack already waiting and just update that one
		deque<SelectiveRepeat::Segment>::iterator it;
		for (it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
			if(it->data.size() == 0) {
				ackUpdated = true;
				it->header.ackNumber = mCurrentAck;
				it->needsTransmit = true;
			}
		}


		//If not then add an ack
		if(!ackUpdated) {
			SelectiveRepeat::Segment seg;

			seg.needsTransmit = true;
			seg.header.seqNumber = mCurrentSeq; //Don't increment seq for ctl packets


			sendBuffer.push_back(seg);
		}
#ifdef DEBUG_MOST
		cout << "sendAck() - adding or updating Ack. Size now = "
				<< sendBuffer.size() << endl;
#endif

	} // Guard sendBuffer
}

/****************************************************************/
void SelectiveRepeat::processDataSegment(Segment& seg) {
	{ // Guard receiveBuffer, receiveStream
		GuardMutex G1(&mutexReceiveBuffer);
		GuardMutex G2(&mutexRecieveStream);

		//Only receive if it's one we want. FIXME: not accounting for roll over, but this isn't production code
		if(seg.header.seqNumber < mCurrentAck){
			sendAck();
			return;
		}


		// First, add this segment to the receiveBuffer in order if we have not yet received this segment
		deque<SelectiveRepeat::Segment>::iterator it = receiveBuffer.begin();
		while (it != receiveBuffer.end()
				&& it->header.seqNumber < seg.header.seqNumber) {
			++it;
		}
		//Don't add duplicates, so make sure we're at end or next is strictly greater than the one we're adding
		if (it == receiveBuffer.end()
				|| it->header.seqNumber > seg.header.seqNumber) {
			receiveBuffer.insert(it, seg);
		}
#ifdef DEBUG_MOST
		cout << "Receive Buffer Seqs:\n";
		for (it = receiveBuffer.begin(); it != receiveBuffer.end(); ++it) {
			cout << "Seq = " << it->header.seqNumber << ' ';
		}
		cout << endl;
#endif

		// Second, scan through to find our next expected segment and set mCurrentAck
		for (it = receiveBuffer.begin(); it != receiveBuffer.end(); ++it) {
			// Starting from the beginning, if this is our expected data, then check for more
			// continue until we've reached a data gap or the end.
			if (it->header.seqNumber == mCurrentAck) {
				mCurrentAck++; //Note: this should be the only place this gets edited
			} else {
				break;
			}
		}

		// Now, if we had data that was ack'd push those data packets upstream
		if (it != receiveBuffer.begin()) {
#ifdef DEBUG_MORE
			cout << "We have data to send upstream.\n";
#endif
			deque<SelectiveRepeat::Segment>::iterator donePackets;
			for (donePackets = receiveBuffer.begin(); donePackets != it;
					++donePackets) {
				//Add data to receiveStream
				receiveStream.insert(receiveStream.end(),
						donePackets->data.begin(), donePackets->data.end());
			}
#ifdef DEBUG_MORE
			cout << "Now removing packets from " << receiveBuffer.begin()->header.seqNumber << " to but not including " << ((it == receiveBuffer.end())? -1 : it->header.seqNumber) << endl;
#endif
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
		for (it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {

			//If this is a last logical segment and it is now acked, signal the guy waiting
			if (it->isLastLogicalSegment
					&& it->header.seqNumber < seg.header.ackNumber) {
				mOutstandingSegments--;
				{ // Guard on LastLogicalSegmentAckedOrRetxLimit
					GuardMutex g(&mutexLastLogicalSegmentAckedOrRetxLimit);
#ifdef DEBUG_MOST
					cout << "Signaling the guy waiting that the last logical segment was ack'd\n";
#endif

					//Hang out until the last logical segment is acked, or we reach a retx limit on a segment
					pthread_cond_signal(
							&condLastLogicalSegmentAckedOrRetxLimit);

				} // Guard on LastLogicalSegmentAckedOrRetxLimit
			}

			if (seg.header.ackNumber <= it->header.seqNumber)
				break;
		}
		//If this acked some of our data, remove it from the buffer
		if (it != sendBuffer.begin()) {
			sendBuffer.erase(sendBuffer.begin(), it);
		}

		// 2. Check for a duplicate ack (Non-data packet with same ack as last packet)
		if (seg.data.size() == 0 && seg.header.ackNumber == mLastReceivedAck) {
			//find the data packet that the far side is missing and mark for retransmit
			deque<SelectiveRepeat::Segment>::iterator it;
			for (it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {

				//Don't care about giving up here since we are talking to the other side
				if (seg.header.ackNumber == it->header.seqNumber) {
					it->needsRetransmit = true;
					it->retxCount++;
				}
			}
		}

		// 3. Update the state
		mLastReceivedAck = seg.header.ackNumber;
		mFarWindowsize = seg.header.windowSize;

	} // Guard sendBuffer, StateEdit

}

namespace {

/****************************************************************/
void *SenderThreadFunction(void *sr) {
	SelectiveRepeat *me = (SelectiveRepeat*) sr;
#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::SenderThreadFunction - thread started\n";
#endif
	while (true) {

		{ // Guard sendBuffer
			GuardMutex Guard(&mutexSendBuffer);

			deque<SelectiveRepeat::Segment>::iterator it;
			for (it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
				if (it->needsTransmit || it->needsRetransmit) {

					//If we have too much outstanding data, don't send this data packet, but keep looking for any ctl packets
					int realWindow = ((int)me->mFarWindowsize - ((int)me->mOutstandingSegments));
					if (it->data.size()	&& (realWindow <= 0)) {

						continue;
					} // -1 is so we don't count this packet that's now in the buffer

					//Send the segment
					me->sendSegment(*it);

					uint32_t sendTime = Util::getCurrentTimeMs();

					//If that was a retransmit, then go and update the send times of all later packets
					if(it->needsRetransmit) {
						deque<SelectiveRepeat::Segment>::iterator jt;
						for (jt = it; jt != sendBuffer.end(); ++jt) {
							jt->sendTime = sendTime;
						}
					}

					//Update the segment flags
					it->sendTime = sendTime;
					it->needsTransmit = false;
					it->needsRetransmit = false;
					// If ack, don't keep it
					if (!it->data.size()) {

						//Note: this invalidates all iterators, so break out and loop again
						sendBuffer.erase(it);
						break;
					}

				}
			}

			//If we're being told to finish - make sure we sent all our acks, and then be done
			if (finishSending) {
				// If we have nothing, just be done
				if (sendBuffer.size() == 0) {
					return NULL;
				}
				// Otherwise make one last attempt and then be done
				else {
					deque<SelectiveRepeat::Segment>::iterator it;
					for (it = sendBuffer.begin(); it != sendBuffer.end();
							++it) {
						if (it->needsTransmit) {
							//Send the segment
							me->sendSegment(*it);
						}
					}
					return NULL;
				}
			}
		} // Guard sendBuffer

		//If we didn't have data, then chill for a bit
		usleep(SEND_SLEEP_US);
	}
	return NULL;
}

/****************************************************************/
void *ReceiverThreadFunction(void *sr) {
	SelectiveRepeat *me = (SelectiveRepeat*) sr;

#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::ReceiverThreadFunction - thread started\n";
#endif

	//Create space to drop data
	char receiveArray[me->mSocket.getMaxSegmentSize(HEADERSIZE) + HEADERSIZE];
	memset(receiveArray, 0, sizeof(receiveArray));
	int recvBytes = 0;

	// Do this forever
	while (true) {

		//Only go through the trouble if we have data
		if (!me->mSocket.hasData()) {
			continue;
		}

		//If this is the first receive, then set the far side address/port
		if (me->mDestinationAddress == "" || me->mDestinationPort == 0) {
			recvBytes = me->mSocket.recvFrom(receiveArray, sizeof(receiveArray),
					&me->mDestinationAddress, &me->mDestinationPort);
		} else {
			recvBytes = me->mSocket.recvFrom(receiveArray,
					sizeof(receiveArray));
		}

		//Create a workable object
		SelectiveRepeat::Segment seg(receiveArray, recvBytes);
#ifdef DEBUG_MOST
		cout << "\nreceiveSegment():\n";
		cout << "  Seq = " << seg.header.seqNumber << endl;
		cout << "  Ack = " << seg.header.ackNumber << endl;
		cout << "  Win = " << seg.header.windowSize << endl;
		cout << "  Datalen = " << seg.data.size() << endl;
		//Util::dump(seg.data);
#endif


		me->processSegmentHeader(seg);

		//If there's data, put in receive buffer
		if (seg.data.size()) {
			me->processDataSegment(seg);
		}

		usleep(DATA_WAIT_SLEEP_US);
	}

	return NULL;
}

/****************************************************************/
void *MonitorThreadFunction(void *sr) {
#ifdef DEBUG_INFO
	cout << "SelectiveRepeat::MonitorThreadFunction - thread started\n";
#endif

	// Do this forever
	while (true) {

		//Chill for a bit
		usleep(MONITOR_SLEEP_US);

		{ //Guard sendBuffer
			GuardMutex Guard(&mutexSendBuffer);
			//See if any packets need retransmitting
			deque<SelectiveRepeat::Segment>::iterator it;
			for (it = sendBuffer.begin(); it != sendBuffer.end(); ++it) {
				if (!it->needsTransmit && !it->needsRetransmit && (Util::getCurrentTimeMs() - it->sendTime)
						> RETX_TIMEOUT_MS) {
					if (it->retxCount < RETX_LIMIT) {
#ifdef DEBUG_MORE
						cout << "Timeout! Retransmitting " << it->header.seqNumber << " len = " << it->data.size() << endl;
#endif
						it->needsRetransmit = true;
						it->retxCount++;
						break;
					} else { // Give up
						{ // Guard on LastLogicalSegmentAckedOrRetxLimit
							GuardMutex g(
									&mutexLastLogicalSegmentAckedOrRetxLimit);
#ifdef DEBUG_MORE
							cout << "Retransmission Limit! Signal To Abort!\n";
#endif
							//Hang out until the last logical segment is acked, or we reach a retx limit on a segment
							pthread_cond_signal(
									&condLastLogicalSegmentAckedOrRetxLimit);

							sleep(1);
							return NULL;
						} // Guard on LastLogicalSegmentAckedOrRetxLimit
					}
				}
			}
		} //Guard sendBuffer

	}
	return NULL;
}
}

