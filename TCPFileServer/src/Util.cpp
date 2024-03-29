/*
 * Util.cpp
 */

#include "Util.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <stdint.h>

using namespace std;





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


