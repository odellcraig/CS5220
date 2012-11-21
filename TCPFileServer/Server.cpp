/*
 * Server.cpp
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */

//For threads
#include <pthread.h>

#include "Server.h"
#include "SendRecv.h"
#include "Socket.h"
#include "Util.h"
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

Server::Server(string _key_file, string _port) : key_file(_key_file)
{
	port = Util::toShort(_port);
}


void Server::start()
{
	//Create, bind, listen
	Socket server_sock;
	if(!server_sock.create())
		throw "Error: server socket error when creating socket.";
	if(!server_sock.bind(port))
		throw "Error: server socket error when binding to port.";
	if(!server_sock.listen())
		throw "Error: server socket error when listening.";

	cout << "Server listening for requests on port: " << port << endl;

	//Accept clients and spin out threads
	while(true)
	{
		Socket new_client;
		if(!server_sock.accept(new_client))
			throw "Error: server socket error while accepting new connection.";


		//Will be deleted by created thread
		SendRecv *sendrecv = new SendRecv(new_client, key_file);

		//TODO: Here we will spin thread to deal with new connection
		cout << "Creating receiver." << endl;

		pthread_t thread;
		if(pthread_create(&thread,NULL, Server::serv, (void *)sendrecv))
			throw "Error: an error occurred when creating the receive thread";
		cout << "Just created new receiver thread" << endl;
		pthread_join(thread, NULL);
		//Receiver thread will handle transfer

	}
}



/**
 * Handles client connection as an independent thread
 * Takes in an instance of SendRecv that holds the socket, key_file, etc.
 */

void *Server::serv(void *r)
{
	//Get the package of stuff (i.e socket, key_file, ...)
	SendRecv *sendrecv = (SendRecv*)r;
	Socket &client_sock = sendrecv->getSocket();

	//Get filename, size, and data
	string file_name = sendrecv->recvString();    //Get filename
	unsigned int file_size = sendrecv->recvInt(); //Get file size
	deque<unsigned char> data_buffer;								 //Get file data
	sendrecv->recvData(data_buffer, file_size);		 //Get file data


	//Dump to output file
	cout << "Creating output file: " << file_name << endl;
	ofstream outFile(file_name.c_str(), ios::out|ios::binary);
	if(!outFile)
		throw "Error: output file was not properly opened";
	for(deque<unsigned char>::iterator it = data_buffer.begin(); it != data_buffer.end(); ++it)
		outFile << *it;

	//Clean up
	client_sock.close();
	delete sendrecv;
	return NULL;
}
