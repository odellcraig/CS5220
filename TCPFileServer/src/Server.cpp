/*
 * Server.cpp
 */

//For threads
#include <pthread.h>

//Local includes
#include "Server.h"
#include "SendRecv.h"
#include "Socket.h"
#include "Util.h"

//Cpp includes
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

//Server without encryption
Server::Server(string iPort) {
	mPort = Util::toShort(iPort);
}


void Server::start() {
	//Create, bind, listen
	Socket serverSocket;
	if (!serverSocket.create())
		throw string(
				"Error: create - server socket error when creating socket.");
	if (!serverSocket.bind(mPort))
		throw string("Error: bind - server socket error when binding to port.");
	if (!serverSocket.listen())
		throw string("Error: listen - server socket error when listening.");

	cout << "Server listening for requests on port: " << mPort << endl;

	//Accept clients and spin out threads
	while (true) {
		Socket newClient;
		if (!serverSocket.accept(newClient))
			throw "Error: accept - server socket error while accepting new connection.";

		//Will be deleted by created thread
		SendRecv *sendrecv = new SendRecv(newClient);

		cout << "Creating thread to handle client connection" << endl;

		pthread_t thread;
		if (pthread_create(&thread, NULL, Server::serv, (void *) sendrecv))
			throw "Error: an error occurred when creating the receive thread";
		cout << "Just created new receiver thread" << endl;

		//Keep it simple for now and join to thread
		pthread_join(thread, NULL);
		//Receiver thread will handle transfer
	}
}

/**
 * Handles client connection as an independent thread
 * Takes in an instance of SendRecv that holds the socket, keyFile, etc.
 */

void *Server::serv(void *r) {
	//Get the package of stuff (i.e socket, key_file, ...)
	SendRecv *sendrecv = (SendRecv*) r;
	Socket &clientSocket = sendrecv->getSocket();

	//Get filename, size, and data
	string fileName = sendrecv->recvString(); //Get filename

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

		//Send the length of the file
		sendrecv->sendInt(dataBuffer.size());

		//Send file
		sendrecv->sendData(dataBuffer);

	} else {
		cerr << "Error: opening file - " << inFile << " aborting";
	}

	//Clean up
	clientSocket.close();
	delete sendrecv;
	return NULL;
}
