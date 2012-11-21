/*
 * SendRecv.cpp
 *
 *  Created on: Mar 11, 2011
 *      Author: codell
 */

#include "Socket.h"
#include "SendRecv.h"
#include "Encryption.h"

#include <string>
#include <deque>
#include <sstream>
using namespace std;




SendRecv::SendRecv(Socket &_sock, std::string key_file) : sock(_sock), encrypt_decrypt(key_file)
{}

Socket &SendRecv::getSocket()
{
	return sock;
}


void SendRecv::sendString(string sendStr)
{
	if(!sock.send(sendStr))
		throw string("Error: sending - ") + sendStr;
	//Recv ACK that file name was acquired
	getACK();
}

void SendRecv::sendInt(int i)
{
	ostringstream os;
	os << i;
	sendString(os.str());
}

void SendRecv::sendData(deque<unsigned char> &data)
{
	//First encrypt the data
	encrypt_decrypt.Encrypt(data);

	//Buffer for sending data
	deque<unsigned char> send_buffer;
	//cout << "Sending data: " << endl;
	for(unsigned int i = 0; i < data.size(); ++i)
	{
		send_buffer.push_back(data[i]);
		//If we have lots of data send in parts
		if(((i+1)%MAXSEND) == 0)
		{
			int bytes_sent = sock.send(send_buffer);
			//cout << "i = " << i << endl;
			//cout << "Bytes sent = " << bytes_sent << endl;
			send_buffer.erase(send_buffer.begin(), send_buffer.begin()+bytes_sent);
			getACK();
		}
	}
	//cout << "Size of remaining data: " << send_buffer.size() << endl;

	//Send the remaining part of the data
	if(!sock.send(send_buffer))
		throw "Error: sending data.";
	getACK();
}



string 	SendRecv::recvString()
{
	string receive_buffer = "";
	if(!sock.recv(receive_buffer))
		throw "Error: problem when receiving string.";
	sendACK();
	return receive_buffer;
}

int		SendRecv::recvInt()
{
	stringstream ss;
	string integer = recvString();
	ss << integer;
	int returnint = 0;
	ss >> returnint;
	return returnint;
}

void 	SendRecv::recvData(deque<unsigned char> &data_buffer, unsigned int size)
{
	//Get the data
	data_buffer.erase(data_buffer.begin(), data_buffer.end());
	deque<unsigned char> current_data;
	while(size > data_buffer.size())
	{
		int bytes_recv = sock.recv(current_data);
		if(!bytes_recv)
				throw "Error: problem when receiving data.";
		data_buffer.insert(data_buffer.end(), current_data.begin(), current_data.end());
		//cout << "Bytes Received: " << bytes_recv << " " << data.size() << endl;
		if((data_buffer.size() % MAXRECV) == 0)
		{
			sendACK();
		}
		//cout << "Bytes Remaining: " << filesize-data.size() << "   " << filesize << "   " <<  data.size() << endl;
	}
	sendACK(); //Send ack for last bytes
	encrypt_decrypt.Decrypt(data_buffer);
	return;
}





void SendRecv::sendACK()
{
	if(!sock.send("ACK"))
		throw "Error: problem sending ACK.";
}






void SendRecv::getACK()
{
	string ackBuffer = "";
	while (ackBuffer != "ACK")
	{
		if(!sock.recv(ackBuffer))
			throw "Error: receiving ACK.";
	}
}
