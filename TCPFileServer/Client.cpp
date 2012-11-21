/*
 * Client.cpp
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */


#include "Client.h"
#include "SendRecv.h"
#include "Socket.h"
#include "Util.h"
#include <string>
#include <sstream>
#include <fstream>
using namespace std;

Client::Client(string _key_file, string _host, string _serverPort) : key_file(_key_file), host(_host)
{
	serverPort = Util::toShort(_serverPort);
}

bool Client::send(string data_file_name)
{
	//Create Socket
	Socket sock;
	if(!sock.create())
		throw "Error: socket creation failed.";
	//Connect Socket
	if(!sock.connect(host, serverPort))
		throw "Error: failed to connect.";


	//Get the data from the file
	//**Break into new method
	ifstream inFile(data_file_name.c_str());
	/* Read in the data from the file */
	deque<unsigned char> data_buffer;
	int i = 0;
	while(inFile){
		char temp;
		inFile.read(&temp,1);
		if(inFile)
			data_buffer.push_back(temp);
		i++;
	}




	SendRecv sender(sock, key_file);
	//Send file name
	sender.sendString(data_file_name);

	//Send file size
	sender.sendInt(data_buffer.size());

	//Send file
	sender.sendData(data_buffer);

	sock.close();
	return true;
}
