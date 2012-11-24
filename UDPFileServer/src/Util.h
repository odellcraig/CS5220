/*
 * Util.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef UTIL_H
#define UTIL_H
#include <sstream>
#include <string>
#include <deque>
#include <stdint.h>

#define SIZE_INT 4
#define SIZE_SHORT 2
#define PSEUDO_HEADER_SIZE 12

class Util {
public:
	static uint32_t getCurrentTimeMs(void);
	static uint16_t toShort(const std::string &);
	static uint32_t toInt(const std::string &);
	///Util: dump raw data
	static void dump(std::deque<unsigned char> s);
};

#endif /* UTIL_H */
