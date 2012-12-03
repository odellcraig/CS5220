/*
 * Client.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */


#include "Client.h"
#include "ARQBase.h"
#include "StopAndWait.h"
#include "SelectiveRepeat.h"
#include "UDPSocket.h"
#include "Util.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

Client::Client(ARQBase::ARQType arqType) :
		mARQType(arqType) {
}

bool Client::getFile(string dataFileName, uint32_t iDropPercentage, string host, uint16_t port, ofstream &traceFile)
{
	//Create Socket
	UDPSocket socket(iDropPercentage);
	if(!socket.createSocket()){ throw string("Error: create - socket creation failed."); }

	//Create this on the heap so we can point a base at any one of the children
	ARQBase *sendRecv;
	if(mARQType == ARQBase::ARQTypeStopAndWait) {
		sendRecv = new StopAndWait(socket, host, port, traceFile);
	}
	if(mARQType == ARQBase::ARQTypeSelectiveRepeat || mARQType == ARQBase::ARQTypeGoBackN) {
		sendRecv = new SelectiveRepeat(socket, host, port, traceFile);
	}

	cout << "Client - UDP Socket Created.\n";
	cout << "Client - Server Name: " << host << " Server Port: " << port << '\n';

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
		cout << "Creating output file: " << dataFileName << '\n';
		ofstream outFile(dataFileName.c_str(), ios::out|ios::binary);
		if(!outFile)
			throw string("Error: output file was not properly opened");
		for(deque<unsigned char>::iterator it = dataBuffer.begin(); it != dataBuffer.end(); ++it)
			outFile << *it;
	}
	catch(string &err) {
		cerr << "Client - An error occurred:\n";
		cerr << "  Server  = " << host << '\n';
		cerr << "  Port    = " << port << '\n';
		cerr << "  ARQType = " << mARQType << '\n';
		cerr << "Error: " << err << '\n';
	}



	//Cleanup
	sendRecv->close();
	socket.close();
	delete sendRecv;

	// Flush
	traceFile << endl;

	return true;
}
