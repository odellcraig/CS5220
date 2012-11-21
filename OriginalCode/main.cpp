/*
 * main.cpp
 *
 *  Created on: Feb 1, 2011
 *      Author: codell
 */



/* Cpp includes */
#include <iostream>
#include <string>
#include <sstream>

/* Clib includes */
#include <cstdlib>
#include <cassert>


/* Local includes */
#include "Server.h"
#include "Client.h"


//#define debug


using namespace std;
//Static definitions
#define CLIENT_ARG_COUNT 6
#define SERVER_ARG_COUNT 4



void printUsage(const char *filename)
{
	ostringstream os;
	os << "Usage:- Server\n" << filename << " -s key_file port-number\n";
	os << "Usage:- Client\n" << filename << " -c  key_file  host-address server-port data_file_name";
	cerr << os.str() << endl;
}



int main(int argc, char **argv)
{

	//Error on incorrect number of arguments -- should throw error here but following guide
	if(argc != CLIENT_ARG_COUNT && argc != SERVER_ARG_COUNT)
	{
		printUsage(argv[0]);
		exit(-1);
	}

	/* Make sure purpose given -- i.e. either receiver or sender */
	string purpose(argv[1]);
	if(!(purpose == "-c" || purpose == "-s"))
	{
		printUsage(argv[0]);
		exit(-1);
	}

	/* If Operating as a Server */
	if(purpose == "-s")
	{
		//Deal with the input arguments
		string key_file(argv[2]);
		string port_number(argv[3]);
		Server server(key_file, port_number);
		try
		{
			server.start();
		}
		catch(const char *Error){
			cerr  << Error << endl;
			exit(-1);
		}
	}
	if(purpose == "-c") //If Operating as Client
	{
		//Deal with the input arguments
		string key_file(argv[2]);
		string serverAddr(argv[3]);
		string serverPort(argv[4]);
		string data_file_name(argv[5]);
		Client client(key_file, serverAddr, serverPort);
		try
		{
			if(client.send(data_file_name))
				cout << "Sent Data\n";
		}
		catch(const char *Error)
		{
			cerr << Error << endl;
			exit(-1);
		}
	}
	return 0;
}

