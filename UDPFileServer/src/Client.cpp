/*
 * Client.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */


#include "Client.h"
#include "ARQBase.h"
#include "StopAndWait.h"
#include "GoBackN.h"
#include "SelectiveRepeat.h"
#include "UDPSocket.h"
#include "Util.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

Client::Client(std::string iServerName, std::string iServerPort,
		ARQBase::ARQType arqType) :
		mServerName(iServerName), mARQType(arqType) {
	mServerPort = Util::toShort(iServerPort);
}

bool Client::getFile(string dataFileName, uint32_t iDropPercentage)
{
	//Create Socket
	UDPSocket socket(iDropPercentage);
	if(!socket.createSocket()){ throw string("Error: create - socket creation failed."); }

	//Create this on the heap so we can point a base at any one of the children
	ARQBase *sendRecv;
	if(mARQType == ARQBase::ARQTypeStopAndWait) {
		sendRecv = new StopAndWait(socket, mServerName, mServerPort);
	}
	//TODO: Add the analogous GoBackN and SelectiveRepeat ARQ's here

	cout << "Client - UDP Socket Created.\n";
	cout << "Client - Server Name: " << mServerName << " Server Port: " << mServerPort << '\n';

	try {
		//Send file name
		sendRecv->sendString(dataFileName);

		cout << "Client - Filename sent to server: "<< dataFileName <<'\n';

		//Receive the file size
		int fileSize = sendRecv->recvInt();

		cout << "Client - Received file size of " << fileSize << '\n';

		//Receive the data
		deque<unsigned char> dataBuffer;
		sendRecv->recvData(dataBuffer, fileSize);

		cout << "Client - Received data.\n";

		//Dump to output file
		cout << "Creating output file: " << dataFileName << endl;
		ofstream outFile(dataFileName.c_str(), ios::out|ios::binary);
		if(!outFile)
			throw string("Error: output file was not properly opened");
		for(deque<unsigned char>::iterator it = dataBuffer.begin(); it != dataBuffer.end(); ++it)
			outFile << *it;
	}
	catch(string &err) {
		cerr << "Client - An error occurred:\n";
		cerr << "  Server  = " << mServerName << '\n';
		cerr << "  Port    = " << mServerPort << '\n';
		cerr << "  ARQType = " << mARQType << '\n';
		cerr << "Error: " << err << '\n';
	}


	//Cleanup
	socket.close();
	delete sendRecv;
	return true;
}
