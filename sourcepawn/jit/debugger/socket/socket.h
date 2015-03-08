#ifndef SOCKET_H_
#define SOCKET_H_

#include <string>

#if defined WIN32
#include <winsock.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

using namespace std;

// Max. connections
const int MAXCONNECTIONS = 5;
// Max. amount of data which can be received at once
const int MAXRECV = 1024;

class Socket {
private:
	// Socket descriptor
	int m_sock;
	sockaddr_in m_addr;

public:
	Socket();
	virtual ~Socket();

	// Socket creation - TCP
	bool create();

	// passive connections
	bool bind(const int port);
	bool listen() const;
	bool accept(Socket&) const;

	// active connection
	bool connect(const string host, const int port);
	// data sending and retrieval
	bool send(const string) const;
	int recv(string&) const;

	// close the socket
	bool close() const;
	// WSAcleanup() windows api stuff
	void cleanup() const;
	bool is_valid() const { return m_sock != -1; }
};

// Exception class
class SockExcept {
private:
	string except;

public:
	SockExcept(string s) : except(s) {};
	~SockExcept() {};
	string get_SockExcept() { return except; }
};

#endif
