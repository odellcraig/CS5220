/*
 * Server.cpp
 *
 *  Authors: Craig Odell and Steven Wilson
 */


//For threads
#include <pthread.h>

//Local includes
#include "Server.h"
#include "ARQBase.h"
#include "StopAndWait.h"
#include "SelectiveRepeat.h"
#include "UDPSocket.h"
#include "Util.h"

//Cpp includes
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

//Server without encryption
Server::Server(string iPort, ARQBase::ARQType arqType) :
		mARQType(arqType){
	mPort = Util::toShort(iPort);
}


void Server::start(uint32_t iDropPercentage) {

	//Create, bind
	UDPSocket serverSocket(iDropPercentage);
	if (!serverSocket.createAndBind(mPort, NULL)) {
		throw string("Error: create and bind - server socket error when creating socket.");
	}

	ARQBase *sendRecv;
	//Accept client requests
	while (true) {
		try {
			//Create this on the heap so we can point a base at any one of the children
			if(mARQType == ARQBase::ARQTypeStopAndWait) {
				sendRecv = new StopAndWait(serverSocket);
			}
			if(mARQType == ARQBase::ARQTypeSelectiveRepeat || mARQType == ARQBase::ARQTypeGoBackN) {
				sendRecv = new SelectiveRepeat(serverSocket);
			}


			cout << "Server - Waiting for request on port: " << mPort << '\n';

			//Get the filename - since we don't know the end-destination this will set the destination
			string fileName = sendRecv->recvString(); //Get filename

			cout << "Server - Received file name: " << fileName << '\n';


			//Open the file
			ifstream inFile(fileName.c_str(), ios::in | ios::binary);

			if (inFile) {
				//Get the file data and put into dataBuffer
				deque<unsigned char> dataBuffer;
				int i = 0;
				while (inFile) {
					char temp;
					inFile.read(&temp, 1);
					if (inFile)
						dataBuffer.push_back(temp);
					i++;
				}

				cout << "Server - File successfully opened.\n";

				//Send the length of the file
				sendRecv->sendInt(dataBuffer.size());

				cout << "Server - Sent file size of " << dataBuffer.size() << '\n';


				//Send file
				sendRecv->sendData(dataBuffer);

				cout << "Server - Sent file data.\n";


			} else {
				cerr << "Server Error: opening file - " << inFile << " aborting";
			}

			sendRecv->close();

			delete sendRecv;
		}
		catch(string &err) {
			cerr << "An error occurred:\n";
			cerr << "  Port    = " << mPort << '\n';
			cerr << "  ARQType = " << mARQType << '\n';
			cerr << "Error: " << err << '\n';

			sendRecv->close();
			serverSocket.close();
			delete sendRecv;
		}
	}
	serverSocket.close();

}
