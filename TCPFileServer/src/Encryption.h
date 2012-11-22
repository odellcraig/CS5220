/*
 * Encryption.h
 *
 */

#ifndef ENCRYPTION_H_
#define ENCRYPTION_H_


#define SHORT_SIZE 2
#include <string>
#include <deque>


class Encryption {
public:
	Encryption(std::string _key_file);
	//Default dtor, copy ctor, and operator= are fine

	///Encrypt the data given in the deque
	void Encrypt(std::deque<unsigned char> &);

	///Decrypt the data given in the deque
	void Decrypt(std::deque<unsigned char> &);


private:
	void get_key();
	void getNextChars(std::deque<unsigned char> &data, unsigned int i, unsigned char &L, unsigned char &R);
	void packChars(unsigned short s, std::deque<unsigned char> &, unsigned int i);
	unsigned short getShort(unsigned char L, unsigned char R);
	unsigned short recursiveEncrypt(unsigned char L, unsigned char R, std::deque<unsigned char> &key, unsigned int i);
	unsigned short recursiveDecrypt(unsigned char L, unsigned char R, std::deque<unsigned char> &key, unsigned int i);
	std::string key_file;
	std::deque<unsigned char> key;
};

#endif /* ENCRYPTION_H_ */
