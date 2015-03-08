// socket.cpp
#include <cstdlib>

#if defined WIN32
#include <winsock.h>
#include <io.h>
#else
#include <cstring>
#endif

#include <iostream>
#include "socket.h"
using namespace std;

Socket::Socket() : m_sock(0) {
#if defined WIN32
	// init Winsock.DLL
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		throw SockExcept(
			"Error initializing WinSock");
	}
#endif // WIN32
}

Socket::~Socket() {
	if (is_valid())
		close();
}

// Creates the TCP socket
bool Socket::create() {
	m_sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock < 0) {
		throw SockExcept("Error creating a socket");
	}

#ifndef WIN32
	int y=1;
	setsockopt( m_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
#endif
	return true;
}

// Bind the socket to a port
bool Socket::bind(const int port) {
	if (!is_valid()) {
		return false;
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons(port);

	int bind_return = ::bind(m_sock,
		(struct sockaddr *) &m_addr, sizeof(m_addr));
	if (bind_return == -1) {
		return false;
	}
	return true;
}

// Tell the socket to wait for incoming connections
bool Socket::listen() const {
	if (!is_valid()) {
		return false;
	}
	int listen_return = ::listen(m_sock, MAXCONNECTIONS);
	if (listen_return == -1) {
		return false;
	}
	return true;
}

// Get the child socket of incoming connections.
// Blocks until a new connection is established.
bool Socket::accept(Socket& new_socket) const {
	int addr_length = sizeof(m_addr);
#if defined WIN32
	new_socket.m_sock = ::accept(m_sock,
		(sockaddr *)&m_addr, (int *)&addr_length);
#else
	new_socket.m_sock = ::accept(m_sock,
		(sockaddr *)&m_addr, (socklen_t *)&addr_length);
#endif
	if (new_socket.m_sock <= 0)
		return false;
	else
		return true;
}

// Connects the socket to the destination server.
bool Socket::connect(const string host, const int port) {
	if (!is_valid())
		return false;
	struct hostent *host_info;
	unsigned long addr;
	memset(&m_addr, 0, sizeof(m_addr));
	// numerical ip?
	if ((addr = inet_addr(host.c_str())) != INADDR_NONE) {
		memcpy((char *)&m_addr.sin_addr, &addr,
			sizeof(addr));
	}
	else {
		// Host name like "localhost"?
		host_info = gethostbyname(host.c_str());
		if (NULL == host_info) {
			throw SockExcept("Unknown server");
		}
		memcpy((char *)&m_addr.sin_addr, host_info->h_addr,
			host_info->h_length);
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);

	int status = ::connect(m_sock,
		(sockaddr *)&m_addr, sizeof(m_addr));

	if (status == 0)
		return true;
	else
		return false;
}

// Send data via TCP
bool Socket::send(const string s) const {
	int status = ::send(m_sock, s.c_str(), s.size(), 0);
	if (status == -1) {
		return false;
	}
	else {
		return true;
	}
}

// Receive data via TCP
int Socket::recv(string& s) const {
	char buf[MAXRECV + 1];
	s = "";
	memset(buf, 0, MAXRECV + 1);

	int status = ::recv(m_sock, buf, MAXRECV, 0);
	if (status > 0 || status != SOCKET_ERROR) {
		s = buf;
		return status;
	}
	else {
		throw SockExcept("Error in Socket::recv");
		return 0;
	}
}

// Free Winsock.dll
void Socket::cleanup() const {
#if defined WIN32
	/* Cleanup Winsock */
	WSACleanup();
#endif
}

// Close the socket
bool Socket::close() const {
#if defined WIN32
	closesocket(m_sock);
#else
	::close(m_sock);
#endif
	// free winsock.dll on windows
	cleanup();
	return true;
}
