/*
 * Socket.cpp -- Borrowed from http://www.pcs.cnu.edu/~dgame/sockets/socketsC++/Socket.cpp
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */


#include "Socket.h"
#include <string.h>  //for memset
#include <iostream>
#include <string>
#include <deque>
#include <errno.h>
#include <fcntl.h>
using namespace std;


Socket::Socket() :  m_sock ( -1 )
{
	memset ( &m_addr, 0, sizeof ( m_addr ) );
}

Socket::~Socket()
{
	if (is_valid())
		::close ( m_sock );
}


bool Socket::create()
{
	//Get a new socket
	m_sock = socket ( AF_INET,   SOCK_STREAM,   0 );

	if(!is_valid())
		return false;

	// TIME_WAIT
	int on = 1;
	if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
		return false;

	return true;
}



bool Socket::bind ( const int port )
{
	if(!is_valid())
		return false;

	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons ( port );

	int bind_return = ::bind ( m_sock,( struct sockaddr * ) &m_addr, sizeof ( m_addr ) );


	if ( bind_return == -1 )
		return false;

	return true;
}


bool Socket::listen() const
{
	if (!is_valid())
		return false;

	int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


	if ( listen_return == -1 )
		return false;

	return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
	int addr_length = sizeof ( m_addr );
	new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

	if ( new_socket.m_sock <= 0 )
		return false;
	else
		return true;
}


int Socket::send ( const std::string s ) const
{
	int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
	if(status == -1)
		return 0;

	return status;
}


int Socket::send (const std::deque<unsigned char> &buffer) const
{
	char buf[MAXSEND + 1];
	memset(buf, 0, MAXSEND+1);
	for(unsigned int i = 0; i < buffer.size(); ++i)
		buf[i] = buffer[i];

	int status = ::send( m_sock, buf, buffer.size(), MSG_NOSIGNAL );
	if(status == -1)
		return 0;
	return status;
}



int Socket::recv (std::deque<unsigned char> &buffer ) const
{
	char buf [ MAXRECV + 1 ];
	memset ( buf, 0, MAXRECV + 1 );
	buffer.erase(buffer.begin(), buffer.end());

	int status = ::recv ( m_sock, buf, MAXRECV, 0 );
	if ( status == -1 )
	{
		std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
		return 0;
	}
	else if ( status == 0 )
	{
		return 0;
	}
	else
	{
		buffer.insert(buffer.begin(), &buf[0], buf+status);
		return status;
	}
}



int Socket::recv ( std::string& s ) const
{
	char buf [ MAXRECV + 1 ];
	s = "";
	memset ( buf, 0, MAXRECV + 1 );

	int status = ::recv ( m_sock, buf, MAXRECV, 0 );

	if ( status == -1 )
	{
		std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
		return 0;
	}
	else if ( status == 0 )
	{
		return 0;
	}
	else
	{
		s = buf;
		return status;
	}
}



bool Socket::connect ( const std::string host, const int port )
{
	if ( ! is_valid() ) return false;

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons ( port );

	int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

	if ( errno == EAFNOSUPPORT ) return false;

	status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

	if ( status == 0 )
		return true;
	else
		return false;
}


bool Socket::close()
{
	if(!is_valid())
		return false;
	int stat = ::close(m_sock);
	if(stat == 0)
		return true;
	return false;
}
