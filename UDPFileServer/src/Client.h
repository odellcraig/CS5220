/*
 * Client.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "ARQBase.h"

#include <string>

class Client {
public:
	Client(std::string iServerName, std::string iServerPort, ARQBase::ARQType arqType);
	//Default big 3 are fine

	///Send that data file
	bool getFile(std::string dataFileName, uint32_t iDropPercentage);
private:
	std::string mServerName;
	int mServerPort;
	ARQBase::ARQType mARQType;
};

#endif /* CLIENT_H_ */
