/*
 * Client.h
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>

class Client {
public:
	Client(std::string _key_file, std::string _host, std::string _serverPort);
	//Default big 3 are fine

	///Send that data file
	bool send(std::string data_file_name);
private:
	std::string key_file;
	std::string host;
	int serverPort;
};

#endif /* CLIENT_H_ */
