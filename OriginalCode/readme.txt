AUTHOR: Craig Odell
EXE: This readme is intended to explain how to compile and run the UDP_Datagram program with encryption/decryption
Note, UDP_Datagram is both the sender and receiver for ECE456 Lab 1 and Lab 2 and for Encryption for Lab 3. To use 
both the sender and the receiver, one must provide a flag to the command line. Please refer 
to COMMAND-LINE-ARGUMENTS for details

INSTALL/COMPILE:
1. Unpack ServerClient.tgz
2. cd ServerClient
3. make clean
4. make
Executable output: ServerClient

COMMAND-LINE-ARGUMENTS:
Note: the command line arguments for UDP_Datagram are slightly different. For the server,
the first argument must be "-s". For the client, the first argument must be "-c". After
the first argument, the key file name must be provided. Finally, the normal arguments for server/client must be provided:


Usage Sender:
ServerClient -s <key_file> <port number>

Usage Receiver:
ServerClient -c <key_file> <HostAddr> <HostPort> <input file>

Note: the output file from the sender is the input file to the receiver.

Usage Example:
ServerClient -s key_file 5555         				  //Server
ServerClient -c key_file localhost 5555 inFile.bin    //Client

Another Example:
ServerClient -s key_file 5553         				  					//Server-- say on teto
ServerClient -c key_file teto.engr.colostate.edu 5555 PolarBear.jpg     //Client


Note: at the server side, a file is created with the  same name as provided to the client.
Note: this uses multi-threading
Note: this uses TCP (I asked Dr. J and he said it was ok since I wanted to do multiple threads)
Note: One could send multiple files from different clients to the server at the same time

The input files used for testing are included and are: inFile.bin and PolarBear.jpg
