#pragma once

#include <streambuf>
#include <WinSock2.h>
#include <string>
#include <memory>

class TCPStreamBuffer : public std::streambuf
{
public:
	// Create a new, unconnected TCP stream buffer.
	TCPStreamBuffer(std::size_t bufferLength = bufferLength_);

	// Move constructor and move-assignment.
	// The moved-from stream buffer should not be used
	// again.
	
	TCPStreamBuffer(TCPStreamBuffer&& other);
	TCPStreamBuffer& operator=(TCPStreamBuffer&& other);

	// Copy constructor and copy-assignment are disabled.
	TCPStreamBuffer(const TCPStreamBuffer&) = delete;
	TCPStreamBuffer& operator=(const TCPStreamBuffer&) = delete;

	// If this is the last instance to die, WSACleanup() is called.
	~TCPStreamBuffer();

	// Connect to the given hostname and port. Returns true
	// on success, false otherwise.
	bool Connect(const std::string& hostname, const std::string& port);

	// Connect to an existing connection on the given socket.
	// This disconnects from the current connection (if there is any).
	// Returns true on success, false otherwise.
	bool Connect(SOCKET s);
	
	// Disconnect the buffer. Does nothing if not already connected.
	void Disconnect();

	// Returns true if the buffer holds a valid connection, false otherwise.
	bool IsConnected() const { return socket_ != INVALID_SOCKET; }

	// Return the internal socket handle (WinSock2).
	// If not connected, the value is INVALID_SOCKET.
	SOCKET SocketHandle() const { return socket_; }

	// Get the underlying out buffer.
	const char* OutBuffer(std::size_t* len = nullptr) const { 
		if (len) *len = epptr() - pbase();
		return outBuffer_.get(); 
	}

	// Get the underlying in buffer.
	const char* InBuffer(std::size_t* len = nullptr) const { 
		if (len) *len = egptr() - eback();
		return inBuffer_.get();
	}

protected:
	virtual int underflow() override;
	virtual int overflow(int c = EOF) override;
	virtual int sync() override;

private:
	bool Send_(char* buffer, std::size_t len);

	SOCKET socket_ = INVALID_SOCKET;

	// Default buffer length
	static const std::size_t bufferLength_ = 1024;

	std::unique_ptr<char[]> outBuffer_;
	std::size_t outLength_;

	std::unique_ptr<char[]> inBuffer_;
	std::size_t inLength_;
};

class WinSockInitializer {
public:
	void Initialize();
	void Shutdown();
	static WinSockInitializer& Instance() {static WinSockInitializer instance; return instance;}
	
private:
	int refCount_ = 0;
	
	WinSockInitializer() {}
};