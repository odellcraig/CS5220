/*
 * Client.h
 *
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>

class Client {
public:
	Client(std::string iServerName, std::string iServerPort);
	//Default big 3 are fine

	///Send that data file
	bool getFile(std::string dataFileName);
private:
	std::string mServerName;
	int mServerPort;
};

#endif /* CLIENT_H_ */
