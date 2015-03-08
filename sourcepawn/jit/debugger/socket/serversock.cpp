// serversock.cpp
#include "serversock.h"
#include "socket.h"

ServerSock::ServerSock(int port) {
	Socket::Socket();
	if (!Socket::create()) {
		throw SockExcept(
			"ServerSock: Error in Socket::create()");
	}
	if (!Socket::bind(port)) {
		throw SockExcept(
			"ServerSock: Error in Socket::bind()");
	}
	if (!Socket::listen()) {
		throw SockExcept(
			"ServerSock: Error in Socket::listen()");
	}
}

ServerSock::~ServerSock() { }

const ServerSock&
ServerSock::operator << (const std::string& s) const {
	if (!Socket::send(s)) {
		throw SockExcept(
			"ServerSock: Error in Socket::send()");
	}
	return *this;
}

const ServerSock& ServerSock::operator >> (std::string& s) const {
	if (!Socket::recv(s)) {
		throw SockExcept(
			"ServerSock: Error in Socket::recv()");
	}
	return *this;
}

void ServerSock::accept(ServerSock& sock) {
	if (!Socket::accept(sock)) {
		throw SockExcept(
			"ServerSock: Error in Socket::accept()");
	}
}

void ServerSock::close() const {
	if (!Socket::close()) {
		throw SockExcept(
			"ServerSock: Error in Socket::close()");
	}
}
