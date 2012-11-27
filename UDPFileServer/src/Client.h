/*
 * Client.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "ARQBase.h"

#include <string>
#include <stdint.h>

class Client {
public:
	Client(ARQBase::ARQType arqType);
	//Default big 3 are fine

	///Send that data file
	bool getFile(std::string dataFileName, uint32_t iDropPercentage, std::string host, uint16_t port);
private:
	ARQBase::ARQType mARQType;
};

#endif /* CLIENT_H_ */
