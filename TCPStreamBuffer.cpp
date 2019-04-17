#define _WIN32_WINNT 0x501

#include "TCPStreamBuffer.h"
#include <iostream>
#include <windows.h>
#include <WS2tcpip.h>

TCPStreamBuffer::TCPStreamBuffer(std::size_t bufferLength)
	: outLength_(bufferLength), inLength_(bufferLength)
{
	if (outLength_ == 0) outLength_ = 1;
	if (inLength_ == 0) inLength_ = 1;

	outBuffer_ = std::make_unique<char[]>(outLength_);
	inBuffer_ = std::make_unique<char[]>(inLength_);

	setg(inBuffer_.get(), inBuffer_.get(), inBuffer_.get());
	setp(outBuffer_.get(), outBuffer_.get() + outLength_);
	
	WinSockInitializer::Instance().Initialize();
}

TCPStreamBuffer::TCPStreamBuffer(TCPStreamBuffer&& other) 
	: outBuffer_(std::move(other.outBuffer_)),
	inBuffer_(std::move(other.inBuffer_)),
	outLength_(other.outLength_),
	inLength_(other.inLength_),
	socket_(other.socket_)
{
	other.socket_ = INVALID_SOCKET;
	other.outLength_ = 0;
	other.inLength_ = 0;
}

TCPStreamBuffer& TCPStreamBuffer::operator=(TCPStreamBuffer&& other) {
	if (this != &other) {
		outBuffer_ = std::move(other.outBuffer_);
		outLength_ = other.outLength_;
		other.outLength_ = 0;
		
		inBuffer_ = std::move(other.inBuffer_);
		inLength_ = other.inLength_;
		other.inLength_ = 0;
		
		socket_ = other.socket_;
		other.socket_ = INVALID_SOCKET;
	}
	return *this;
}

TCPStreamBuffer::~TCPStreamBuffer() {
	Disconnect();

	// Shut down WinSock2 if this is the last instance of the buffer.
	// If someone else has initialized WinSock2, that's fine; WSACleanup()
	// only decrements an internal counter if WSAStartup() has been called
	// more than once.
	WinSockInitializer::Instance().Shutdown();
}

bool TCPStreamBuffer::Connect(const std::string& hostname, const std::string& port) {
	// Disconnect from any existing connection.
	Disconnect();

	// Reset buffer pointers.
	setg(inBuffer_.get(), inBuffer_.get(), inBuffer_.get());
	setp(outBuffer_.get(), outBuffer_.get() + outLength_);

	// Resolve hostname.
	addrinfo hints;
	addrinfo* result = nullptr;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result) != 0) {
		return false;
	}

	// Create the socket used for transmission.
	socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (socket_ == INVALID_SOCKET) {
		freeaddrinfo(result);
		return false;
	}

	// Attempt to connect to the destination.
	if (connect(socket_, result->ai_addr, result->ai_addrlen) != 0) {
		freeaddrinfo(result);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
		return false;
	}

	freeaddrinfo(result);
	return true;
}

bool TCPStreamBuffer::Connect(SOCKET s) {
	// Disconnect from any existing connection.
	Disconnect();

	// Reset buffer pointers.
	setg(inBuffer_.get(), inBuffer_.get(), inBuffer_.get());
	setp(outBuffer_.get(), outBuffer_.get() + outLength_);
	
	// Set the new socket.
	socket_ = s;
	
	// Return true if a valid socket was passed.
	return socket_ != INVALID_SOCKET;
}

void TCPStreamBuffer::Disconnect() {
	if (socket_ != INVALID_SOCKET) {
		// 1. Flush the out buffer to send any remaining data.
		// 2. Call shutdown() to initiate a graceful disconnection request.
		// 3. Close the socket.
		sync();
		shutdown(socket_, SD_BOTH);
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

int TCPStreamBuffer::underflow() {
	// Underflow is called when there is nothing left in the input buffer.
	// Underflow() will then receive some data from the connection
	// and put it in the buffer, where a stream can access it.

	// We're not connected or we have no buffer, so return EOF.
	if (socket_ == INVALID_SOCKET || !inBuffer_.get()) {
		return std::char_traits<char>::eof();
	}

	// Read as much as we can into the buffer.
	int bytesRead = recv(socket_, inBuffer_.get(), inLength_, 0);

	if (bytesRead <= 0) {
		Disconnect();
		return std::char_traits<char>::eof();
	}

	// Set the input buffer's pointers to reflect the change and return
	// the first character in the buffer.
	setg(inBuffer_.get(), inBuffer_.get(), inBuffer_.get() + bytesRead);
	return std::char_traits<char>::to_int_type(inBuffer_.get()[0]);
}

int TCPStreamBuffer::overflow(int c) {
	if (socket_ == INVALID_SOCKET || !outBuffer_.get())
		return std::char_traits<char>::eof();

	// Flush the buffer and reset the put pointers.
	//

	if (sync() != 0) {
		Disconnect();
		return std::char_traits<char>::eof();
	}

	setp(outBuffer_.get(), outBuffer_.get() + outLength_);

	// Insert the new character.
	if (c != std::char_traits<char>::eof())
		sputc(c);

	return c;
}


int TCPStreamBuffer::sync() {
	// Flush the buffer and reset the put pointers.
	if (!Send_(outBuffer_.get(), pptr() - pbase()))
		return -1;

	setp(outBuffer_.get(), outBuffer_.get() + outLength_);
	return 0;
}

bool TCPStreamBuffer::Send_(char* buffer, std::size_t len) {
	if (socket_ == INVALID_SOCKET) return false;

	// Send the contents of the buffer, repeating calls to
	// send() in case not everything gets sent at once.
	std::size_t bytesSent = 0;
	while (bytesSent < len) {
		int res = send(socket_, buffer + bytesSent, len - bytesSent, 0);
		if (res != SOCKET_ERROR) {
			bytesSent += res;
		}
		else {
			return false;
		}
	}

	return true;
}

void WinSockInitializer::Initialize() {
	if (refCount_ == 0) {
		WSAData data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			throw std::runtime_error("WinSockInitializer::Initialize");
		}
	}

	++refCount_;
}

void WinSockInitializer::Shutdown() {
	--refCount_;
	
	if (refCount_ == 0) {
		WSACleanup();
	}
}
