/*
 *      Authors: Craig Odell and Steven Wilson
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
			<< "\n\t-c              #Act as client"
			<< "\n\t-p portNumber	#Use this port\n"
			<< "\n\t<Remote Server Name> <File Name>\n";
	os << "Usage:- Server (" << file << "):"
			<< "\n\t-s              #Act as server"
			<< "\n\t-p portNumber	#Use this port\n";
	os << "\nClient Example:\n>" << file
			<< " -c -p 1234 192.168.0.10 MyFile.txt\n";
	os << "Server Example:\n>" << file << " -s -p 1234\n";
	cerr << os.str() << endl;
}

int main(int argc, char **argv) {

	string binaryName = argv[0];
	char optChar;
	bool isServer = false;
	bool isClient = false;
	string serverName = "";
	string transferFileName = "";
	string port = DEFAULT_PORT;
	while ((optChar = getopt(argc, argv, "h?csp:")) != -1) {
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
		Server server(port);
		try {
			server.start();
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

		cout << "Server Name: " << serverName << '\n';
		cout << "Server Port: " << port << '\n';

		//Deal with the input arguments
		Client client(serverName, port);
		try {
			if (client.getFile(transferFileName))
				cout << "File Received. Done.\n";
		} catch (const char *Error) {
			cerr << Error << endl;
			exit(-1);
		}
	}
	return 0;
}

