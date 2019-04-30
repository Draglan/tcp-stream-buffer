#ifndef TCP_LISTENER_H
#define TCP_LISTENER_H

#include <string>
#include <WinSock2.h>
#include <cctype>

class TCPStreamBuffer;

class TCPListener {
public:
	// Create a TCP Listener.
	TCPListener();

	// Copy-construction and assignment are disabled.
	TCPListener(const TCPListener&) = delete;
	TCPListener& operator=(const TCPListener&) = delete;
	
	// Move constructor and assignment.
	TCPListener(TCPListener&&);
	TCPListener& operator=(TCPListener&&);
	
	// Destroy the TCP Listener.
	~TCPListener();
	
	// Start listening for connections. Returns false on failure, true otherwise.
	bool Listen(const std::string& port, int maxQueueLength = SOMAXCONN);
	
	// Accept a connection. Returns false if no connection was accepted,
	// true otherwise. Blocks until a connection is accepted or an error
	// occurs. If true is returned, a valid, connected TCPStreamBuffer
	// is returned via the parameter.
	bool Accept(TCPStreamBuffer& stream);
	
	// Disconnect and stop listening.
	void Disconnect();
	
	// True if connected to a valid socket.
	bool IsConnected() const {return socket_ != INVALID_SOCKET;}
	
	// Get the underlying listening socket as a WinSock2 socket.
	SOCKET SocketHandle() const {return socket_;};
	
	
private:
	SOCKET socket_ = INVALID_SOCKET;
};

#endif /* TCP_LISTENER_H */