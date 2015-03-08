#ifndef SERVERSOCK_H
#define SERVERSOCK_H

#include <string>
#include "socket.h"
using namespace std;

class ServerSock : private Socket {
public:
	ServerSock(int port);
	ServerSock(){};

	virtual ~ServerSock();
	// Send data
	const ServerSock& operator << (const string&) const;
	// Receive data
	const ServerSock& operator >> (string&) const;
	// Close the socket
	void close() const;
	// Get the child socket of incoming connections.
	// Blocks until a new connection is established.  
	void accept(ServerSock&);
};
#endif
