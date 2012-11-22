/*
 * Server.h
 *
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <string>


class Server {
public:
	Server(std::string iPort);
	//Default Big three are fine

	///Start the server
	void start();

private:
	///Used as the thread handler
	static void *serv(void *receiver);
	int mPort;
};

#endif /* SERVER_H_ */
