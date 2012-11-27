//============================================================================
// Name        : UDPFileServer.cpp
// Author      : Craig Odell and Steven Wilson
// Version     :
// Copyright   : 
// Description : File server with different ARQ protocols using UDP sockets
//============================================================================


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
#include "Util.h"
// For debugging
//#define debug

using namespace std;

// Use an unnamed namespace for locals
namespace {
const string DEFAULT_PORT = "2523";
const int MANDATORY_ARG_COUNT = 2;
}

void printUsage(string file) {
	ostringstream os;
	os << "Help: -? or -h\n";
	os << "Usage:- Client (" << file << "):"
			<< "\n\t-c              	#Act as client"
			<< "\n\t-p portNumber		#Use this port"
			<< "\n\t-t <ARQ-Type>   	#1 - Stop and Wait, 2 - Go Back N, 3 - Selective Repeat"
			<< "\n\t-d <dropPercentage> #Integer percentage of packets to drop\n"
			<< "\n\t<Remote Server Name> <File Name>\n\n";
	os << "Usage:- Server (" << file << "):"
			<< "\n\t-s              #Act as server"
			<< "\n\t-p portNumber	#Use this port"
			<< "\n\t-t <ARQ-Type>   #1 - Stop and Wait, 2 - Go Back N, 3 - Selective Repeat"
			<< "\n\t-d <dropPercentage> #Integer percentage of packets to drop\n";
	os << "\nClient Example:\n>" << file
			<< " -c -p 1234 -t1 192.168.0.10 MyFile.txt\n";
	os << "Server Example:\n>" << file << " -s -p 1234 -t1\n";
	cerr << os.str() << endl;
}

int main(int argc, char **argv) {

	string binaryName = argv[0];
	char optChar;
	bool isServer = false;
	bool isClient = false;
	string serverName = "";
	string transferFileName = "";
	ARQBase::ARQType arqType = ARQBase::ARQTypeStopAndWait;
	string port = DEFAULT_PORT;
	uint32_t dropPercentage = 0;
	while ((optChar = getopt(argc, argv, "h?csp:t:d:")) != -1) {
		switch (optChar) {
		case '?':
		case 'h':
			// Print usage
			printUsage(binaryName);
			exit(0);
		case 'p':
			// Use this port number
			port = optarg;
			break;
		case 's':
			// Act as the server
			isServer = true;
			break;
		case 'c':
			// Act as the client
			isClient = true;
			break;
		case 't':
			arqType = (ARQBase::ARQType)Util::toInt(optarg);
			break;
		case 'd':
			dropPercentage = Util::toInt(optarg);
			if(dropPercentage > 100) {
				printUsage(binaryName);
				exit(-1);
			}
			break;
		default:
			printUsage(binaryName);
			exit(-1);
		}
	}
	//Jump ahead to the non option arguments (i.e. serverName and transferFileName)
	argc -= optind;
	argv += optind;

	// Make sure we're the server or the client but not both
	if ((isServer && isClient) || !(isServer || isClient)) {
		printUsage(binaryName);
		exit(-1);
	}

	/* If Operating as a Server */
	if (isServer) {
		Server server(port, arqType);
		try {
			server.start(dropPercentage);
		} catch (string &Error) {
			cerr << Error << endl;
			exit(-1);
		}
	}

	/* If Operating as a Client */
	if (isClient) {
		// Basic checks
		if (argc != MANDATORY_ARG_COUNT) {
			printUsage(binaryName);
			exit(-1);
		}

		serverName = argv[0];
		transferFileName = argv[1];

		//Deal with the input arguments
		Client client(arqType);
		try {
			if (client.getFile(transferFileName,dropPercentage, serverName, Util::toShort(port)))
				cout << "File Received. Done.\n";
		} catch (const char *Error) {
			cerr << Error << endl;
			exit(-1);
		}
	}
	return 0;
}
