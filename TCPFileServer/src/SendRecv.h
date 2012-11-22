/*
 * SendRecv.h
 */

#ifndef SENDRECV_H_
#define SENDRECV_H_
#include "Socket.h"
#include <string>
#include <deque>



class SendRecv {
public:
	SendRecv(Socket &_sock);

	Socket &getSocket();

	//Helper methods for easy transfer of data, strings, and ints
	void sendString(std::string sendStr);
	void sendInt(int i);
	void sendData(std::deque<unsigned char> &);

	//Helper methods for easy reception of data, strings, ints
	std::string recvString();
	int			recvInt();
	void 		recvData(std::deque<unsigned char> &, unsigned int size);

private:
	void getACK();
	void sendACK();

	//Hold the socket and the encryption object
	Socket sock;
};

#endif /* SENDRECV_H_ */
