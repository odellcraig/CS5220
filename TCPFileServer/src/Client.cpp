/*
 * Client.cpp
 *
 */


#include "Client.h"
#include "SendRecv.h"
#include "Socket.h"
#include "Util.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

Client::Client(std::string iServerName, std::string iServerPort) : mServerName(iServerName)
{
	mServerPort = Util::toShort(iServerPort);
}

bool Client::getFile(string dataFileName)
{
	//Create Socket
	Socket sock;
	if(!sock.create()){ throw string("Error: create - socket creation failed."); }

	//Connect Socket
	if(!sock.connect(mServerName, mServerPort)) { throw string("Error: connect - failed to connect."); }

	//Create our send/receive helper
	SendRecv sendRecv(sock);

	//Send file name
	sendRecv.sendString(dataFileName);

	//Receive the file size
	int fileSize = sendRecv.recvInt();

	//Receive the data
	deque<unsigned char> dataBuffer;
	sendRecv.recvData(dataBuffer, fileSize);


	//Dump to output file
	cout << "Creating output file: " << dataFileName << endl;
	ofstream outFile(dataFileName.c_str(), ios::out|ios::binary);
	if(!outFile)
		throw string("Error: output file was not properly opened");
	for(deque<unsigned char>::iterator it = dataBuffer.begin(); it != dataBuffer.end(); ++it)
		outFile << *it;


	//Cleanup
	sock.close();
	return true;
}
