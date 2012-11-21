/*
 * Socket.h - from http://www.pcs.cnu.edu/~dgame/sockets/socketsC++/Socket.h
 *
 *  Created on: Mar 6, 2011
 *      Author: codell
 */

// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <deque>
#include <string>
#include <arpa/inet.h>


#define MAXHOSTNAME  200
#define MAXCONNECTIONS  10
#define MAXRECV  1024
#define MAXSEND  1024


class Socket
{
 public:
  Socket();
  virtual ~Socket();

  // Server initialization
  ///Create a socket
  bool create();
  ///Bind to a port
  bool bind ( const int port );

  ///Start listening
  bool listen() const;

  ///Accept a new request
  bool accept ( Socket& ) const;

  ///Close the socket
  bool close();

  // Client initialization
  ///Connect to a server
  bool connect ( const std::string host, const int port );

  // Data Transimission

  //Send or receive data with deque or a string for a message
  int send ( const std::deque<unsigned char> &) const;
  int send ( const std::string ) const;
  int recv ( std::deque<unsigned char> &) const;
  int recv ( std::string& ) const;


  bool is_valid() const { return m_sock != -1; }

 private:
  int m_sock;
  sockaddr_in m_addr;
};


#endif
