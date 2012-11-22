/*
 * Encryption.cpp
 */

#include "Encryption.h"
#include <string>
#include <fstream>
using namespace std;

#define BITS_PER_BYTE 8

/**
 * Constructor takes in key file
 */
Encryption::Encryption(string _key_file) : key_file(_key_file), key(0) {
	get_key();
}


/**
 * Will encrypt the data in place
 */
void Encryption::Encrypt(deque<unsigned char> &data)
{
	for(unsigned int i = 0; i < data.size(); i=i+2)
	{
		//If we have an odd number of bytes, just add on the last one
		if(i != data.size()-1)
		{
			unsigned char L,R;
			L = R = 0;
			unsigned short s;
			getNextChars(data, i, L, R);
			s = recursiveEncrypt(L, R, key, 0);
			packChars(s,data,i);
		}

	}
}

/**
 * Will decrypt the data in place
 */
void Encryption::Decrypt(deque<unsigned char> &data)
{
	for(unsigned int i = 0; i < data.size(); i=i+2)
	{
		//If we have an odd number of bytes, just add on the last one
		if(i != data.size()-1)
		{
			unsigned char L,R;
			L = R = 0;
			unsigned short s;
			getNextChars(data, i, L, R);
			s = recursiveDecrypt(L, R, key, key.size());
			packChars(s,data,i);
		}
	}
}



/** Private Methods **/


/**
 * This gets the bytes for the key from the file and stores it in the member variable, key
 */
void Encryption::get_key()
{
	//Open the file for reading (binary reading)
	ifstream inFile(key_file.c_str(), ios::in|ios::binary);
	if(!inFile)
		throw "Error: input key file was not properly opened";


	/* Read in the data from the file */
	int i = 0;
	while(inFile){
		char temp;
		inFile.read(&temp,1);
		if(inFile)
			key.push_back(temp);
		i++;
	}
}



/**
 * From the data, this will extract the next two characters to be used. Zero pads R if we're at the last element in data (and data is odd bytes)
 */
void Encryption::getNextChars(deque<unsigned char>& data, unsigned int i, unsigned char &L, unsigned char &R)
{
	if(i >= data.size())
		return;

	if(i == data.size()-1)
	{
		L = data[i];
		R = 0;
		return;
	}

	L = data[i];
	R = data[i+1];
}

/**
 * This will take a short and pack it's corresponding bytes into the data (used either after encryption or after decryption)
 */
void Encryption::packChars(unsigned short s, std::deque<unsigned char> &data, unsigned int i)
{
	unsigned char L = (unsigned char)((s&0xFF00)>>BITS_PER_BYTE);
	unsigned char R = (unsigned char)(s&0x00FF);

	if(i >= data.size())
			return;

	if(i == data.size()-1)
	{
	    data[i] = L;
		data.push_back(R);
		return;
	}

	data[i] = L;
	data[i+1] = R;
}


/**
 * Creates a short from the two characters (left = MSB, right = LSB)
 */
unsigned short Encryption::getShort(unsigned char L, unsigned char R)
{
	unsigned short retShort = (unsigned short)(((L<<BITS_PER_BYTE)&0xFF00) + ((R)&0x00FF));
	return retShort;
}



/**
 * Uses recursion to encrypt the two bytes using the xor&swap method
 */
unsigned short Encryption::recursiveEncrypt(unsigned char L, unsigned char R, std::deque<unsigned char> &key, unsigned int i)
{
	if(i == key.size())
		return getShort(L,R);
	unsigned char temp;
	temp = L;
	L = R;
	R = temp;
	L = L ^ key[i];
	return recursiveEncrypt(L, R, key, i+1);
}


/**
 * Uses recursion to decrypt the two bytes using xor&swap method -- needs to go backwards on the key
 */
unsigned short Encryption::recursiveDecrypt(unsigned char L, unsigned char R, std::deque<unsigned char> &key, unsigned int i)
{
	if(i == 0)
		return getShort(L,R);
	unsigned char temp;
	temp = L;
	L = R;
	R = temp;
	R = R ^ key[i-1];
	return recursiveDecrypt(L, R, key, i-1);
}


