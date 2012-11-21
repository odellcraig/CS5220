/*
 * Packet.h
 *
 *  Created on: Feb 1, 2011
 *      Author: codell
 */

#ifndef PACKET_H_
#define PACKET_H_
#include <sstream>
#include <string>
#include <deque>

#define SIZE_INT 4
#define SIZE_SHORT 2
#define PSEUDO_HEADER_SIZE 12

class Util {
public:
	//Static util functions:
	///Pack a short onto the end of the queue
	static void packShort(unsigned short, std::deque<unsigned char> &);
	///Pack an int onto the end of the queue
	static void packInt(unsigned int, std::deque<unsigned char> &);
	///Unpack a short from the front of the deque
	static unsigned short unpackShort(std::deque<unsigned char> &);
	///Calculate one's checksum
	static unsigned short CalculateChecksum(std::deque<unsigned char> data);
	//Util switch byte order
	static unsigned int SwapByteOrder32(unsigned int);
	static unsigned short SwapByteOrder16(unsigned short);
	///Util: calc length (UDP_header_size + data_size
	static unsigned short calcUDPLength(unsigned int data_size);
	static unsigned short toShort(const std::string &);
	static unsigned int toInt(const std::string &);
	///Util: dump raw data
	static void dump(std::deque<unsigned char> s);
};

#endif /* PACKET_H_ */
