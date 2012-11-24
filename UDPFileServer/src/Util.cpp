/*
 * Util.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */


#include "Util.h"
#include <sstream>
#include <cstdio>
#include <iostream>
#include <stdint.h>
#include <sys/time.h>

using namespace std;


uint32_t Util::getCurrentTimeMs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}



uint16_t Util::toShort(const string &s)
{
	stringstream os;
	os << s;
	unsigned short retS = 0;
	os >> retS;
	return retS;
}

uint32_t Util::toInt(const string &s)
{
	stringstream os;
	os << s;
	unsigned int retS = 0;
	os >> retS;
	return retS;
}


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


