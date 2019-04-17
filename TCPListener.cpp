#define _WIN32_WINNT 0x501

#include "TCPListener.h"
#include "TCPStreamBuffer.h"
#include <sstream>
#include <WS2tcpip.h>

TCPListener::TCPListener() {
	WinSockInitializer::Instance().Initialize();
}

TCPListener::TCPListener(TCPListener&& other) 
	: socket_(other.socket_)
{
	other.socket_ = INVALID_SOCKET;
}

TCPListener& TCPListener::operator=(TCPListener&& other) {
	if (this != &other) {
		socket_ = other.socket_;
		other.socket_ = INVALID_SOCKET;
	}
	return *this;
}

TCPListener::~TCPListener() {
	WinSockInitializer::Instance().Shutdown();
}

// Start listening for connections. Returns false on failure, true otherwise.
bool TCPListener::Listen(std::uint16_t port, int maxQueueLength) {
	// Disconnect from any existing connections.
	Disconnect();
	
	// Get connection information about the host.
	addrinfo hints;
	addrinfo* result;
	memset(&hints, 0, sizeof(hints));
	
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	std::stringstream ssPort;
	ssPort << port;
	
	if (getaddrinfo("localhost", ssPort.str().c_str(), &hints, &result) != 0) {
		return false;
	}
	
	// Create the socket with the given configuration.
	socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	if (socket_ == INVALID_SOCKET) {
		freeaddrinfo(result);
		return false;
	}
	
	// Bind the socket to the host.
	if (bind(socket_, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR) {
		freeaddrinfo(result);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
		return false;
	}
	
	freeaddrinfo(result);
	
	// Start listening on the newly bound socket.
	if (listen(socket_, maxQueueLength) != 0) {
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
		return false;
	}
	
	return true;
}

// Accept a connection. Returns false if no connection was accepted,
// true otherwise. Blocks until a connection is accepted or an error
// occurs.
bool TCPListener::Accept(TCPStreamBuffer& stream) {
	SOCKET newsock = accept(socket_, nullptr, nullptr);
	
	if (newsock != INVALID_SOCKET) {
		return stream.Connect(newsock);
	}
	else {
		return false;
	}
}

// Disconnect and stop listening.
void TCPListener::Disconnect() {
	if (IsConnected()) {
		shutdown(socket_, SD_BOTH);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}