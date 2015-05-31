/**
 * Connection
 *
 * @author	Stanislav Nechutný - xnechu01
 */

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include "connection.h"
#define MY_PORT 9314

bool Connection::bindServer(int port)
{
	if( (_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{ /* create socket*/
		return false;
	}

	/*set protocol family to Internet */
	_sin.sin_family = PF_INET;
	/* set port no. */
	_sin.sin_port = htons(port);
	/* set IP addr to any interface */
	_sin.sin_addr.s_addr  = INADDR_ANY;
	if(bind(_socket, (struct sockaddr *)&_sin, sizeof(_sin) ) < 0 )
	{ /* bind error */
		return false;
	}

	if(listen(_socket, 5))
	{ /* listen error*/
		return false;
	}

	return true;
}

bool Connection::connectTo(char* host, int port)
{
	struct hostent *hostPtr;

	/* create socket*/
	if( (_socket = socket(PF_INET, SOCK_STREAM, 0 ) ) < 0)
	{ /* socket error */
		return false;
	}

	/*set protocol family to Internet */
	_sin.sin_family = PF_INET;
	/* set port no. */
	_sin.sin_port = htons(port);
	if( (hostPtr =  gethostbyname(host) ) == NULL)
	{ /* Bad host */
		return false;
	}

	memcpy( &_sin.sin_addr, hostPtr->h_addr, hostPtr->h_length);

	if( connect(_socket, (struct sockaddr *)&_sin, sizeof(_sin) ) < 0 )
	{ /* Connect error */
		return false;
	}

	return true;
}

int Connection::receive(char* message, int length)
{
	/* read message */
	return read(_socket, message, length);
}

bool Connection::send(char* message, int length)
{
	/* Send message */
	if( write(_socket, message, length) < 0 )
	{ /* write error */
		return false;
	}

	return true;
}

void Connection::injectSocket(int sock)
{
	_socket = sock;
}

Connection* Connection::acceptConnection()
{
	int t;
	socklen_t sinlen = sizeof(_sin);
	Connection* result = new Connection;
	if( (t = accept(_socket, (struct sockaddr *) &_sin, &sinlen) ) < 0 )
	{ /* accept error */
		return NULL;
	}
	result->injectSocket(t);

	return result;
}

void Connection::disconnect()
{
	close(_socket);
}
