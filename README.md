## C++ TCP Stream Buffer
This is a simple C++ TCP stream buffer, using WinSock2, that allows you to use IO streams over a TCP connection using the familiar stream syntax we all know and love.

### Usage
#### Stream buffer
    #include <iostream>
	#include <string>
	#include "TCPStreamBuffer.h"
	
	int main() {
		TCPStreamBuffer buf;
		
		if (buf.Connect("www.google.com", "80")) {
			std::iostream stream(&buf);
			
			stream << "GET / HTTP/1.1\r\n";
			stream << "Host: www.google.com\r\n";
			stream << "Connection: close\r\n\r\n";
			stream.flush();
			
			std::string line;
			while (std::getline(stream, line)) {
				std::cout << line << std::endl;
			}
		}
	}

#### Listener
    #include <iostream>
	#include <string>
	#include "TCPListener.h"
	#include "TCPStreamBuffer.h"

	int main() {
		TCPListener listener;

		if (listener.Listen("80")) {
			TCPStreamBuffer buf;

			if (listener.Accept(buf)) {
				std::iostream stream(&buf);

				std::string line;
				while (std::getline(stream, line)) {
					std::cout << line << std::endl;
				}
			}
		}
	}
