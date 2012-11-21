/*
 * Server.h
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <string>


class Server {
public:
	Server(std::string _key_file, std::string _port);
	//Default Big three are fine

	///Start the server
	void start();

private:
	///Used as the thread handler
	static void *serv(void *receiver);

	std::string key_file;
	int port;
};

#endif /* SERVER_H_ */
