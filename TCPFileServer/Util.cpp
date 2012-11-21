/*
 * Packet.cpp
 *
 *  Created on: Feb 1, 2011
 *      Author: codell
 */

#include "Util.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <climits>   //For CHAR_BIT
using namespace std;


//#define debug


#define BITS_PER_BYTE 8
#define UDP_HEADER_SIZE 8
#define PSEUDO_HEADER_SIZE 12




/**
 * Packs a short into data stream
 */
void Util::packShort(unsigned short number, deque<unsigned char> &buffer)
{
	number = SwapByteOrder16(number);
	for(unsigned int i = 0; i < sizeof(unsigned short); ++i)
	{
		unsigned char temp = (unsigned char) (number >> (BITS_PER_BYTE * i)) & 0xFF;
		buffer.push_back(temp);
	}
}


/*
 * Packs an integer into data stream
 */
void Util::packInt(unsigned int number, deque<unsigned char> &buffer)
{
	for(unsigned int i = 0; i < sizeof(unsigned int); ++i)
	{
		unsigned char temp = (unsigned char) (number >> (BITS_PER_BYTE * i)) & 0xFF;
		buffer.push_back(temp);
	}
}


unsigned short Util::unpackShort(deque<unsigned char> &buffer)
{
	unsigned char MSB = buffer[0]; buffer.pop_front();
	unsigned char LSB = buffer[0]; buffer.pop_front();
	unsigned short retShort = ((MSB<<BITS_PER_BYTE)&0xFF00) + ((LSB)&0x00FF);
	return retShort;
}


/**
 * Calculates the checksum for the packet contained in data
 */
unsigned short Util::CalculateChecksum(deque<unsigned char> data)
{
	//Zero padding if necessary
	if(data.size()%2)
		data.push_back(0);

	//Add everything into a uint
	unsigned int checksum = 0;
	for(unsigned int i = 0; i < data.size(); i+=2)
	{
		unsigned short bytes = (((data[i]<<BITS_PER_BYTE)&0xFF00) | (data[i+1] & 0x00FF));
		//unsigned short *bytes = reinterpret_cast<unsigned short *>(&data[i]);
		checksum += (bytes);
	}

	//Get the lower bytes
	unsigned short return_checksum = (checksum & 0xffff);
	//Add on the number of carries
	return_checksum += (checksum >> 16);
	//Return the complement
	return ~return_checksum;
}






/**
 * Util function for swaping byte order
 */
unsigned int Util::SwapByteOrder32(unsigned int v)
{
	return (((v&0x000000FF)<<24)+((v&0x0000FF00)<<8)+
			 ((v&0x00FF0000)>>8)+((v&0xFF000000)>>24));
}
/**
 * Util function for swaping byte order
 */
unsigned short Util::SwapByteOrder16(unsigned short s)
{
	return (((s&0x00FF)<<8) + ((s&0xFF00)>>8));
}



/**
 * Calculates the size to put in the UDP total length field
 */
unsigned short Util::calcUDPLength(unsigned int data_size)
{
	unsigned short total_length = data_size + UDP_HEADER_SIZE;
	return total_length;
}

unsigned short Util::toShort(const string &s)
{
	stringstream os;
	os << s;
	unsigned short retS = 0;
	os >> retS;
	return retS;
}

unsigned int Util::toInt(const string &s)
{
	stringstream os;
	os << s;
	unsigned int retS = 0;
	os >> retS;
	return retS;
}


/*
void Packet::dump(deque<unsigned char> data)
{
	for(unsigned int i = 0; i < data.size(); i+=2)
	{
		printf("%02X%02X ", data[i], data[i+1]);
		//if(((i+1) % 4) == 0)
		//	cout << '\n';
	}
}*/


/**
 * Debug function to dump in hex the data
 */
void Util::dump(deque<unsigned char> data)
{
	for(unsigned int i = 0; i < data.size(); i++)
	{
		printf("%02X ", data[i]);
		if(((i+1) % 4) == 0)
			cout << '\n';
	}
}


