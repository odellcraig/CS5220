/*
 * Server.h
 *
 *  Authors: Craig Odell and Steven Wilson
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "ARQBase.h"

#include <string>
#include <fstream>
#include <stdint.h>


class Server {
public:
	Server(std::string iPort, ARQBase::ARQType arqType);
	//Default Big three are fine

	///Start the server
	void start(uint32_t iDropPercentage, std::ofstream &traceFile);

private:
	///Used as the thread handler
	int mPort;
	ARQBase::ARQType mARQType;
};

#endif /* SERVER_H_ */
