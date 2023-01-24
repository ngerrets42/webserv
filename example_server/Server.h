#ifndef SERVER_H
# define SERVER_H

# include <stdexcept>
# include <cstring>

// networking
# include <unistd.h>
# include <string.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/errno.h>

# include "Connection.h"

#include <iostream>

class Server
{
	public:
	Server()
	{
		// Settings
		const int domain = AF_INET;
		const int type = SOCK_STREAM;
		const int protocol = 0;
		const uint16_t port = 8080;

		std::cout << "Starting server on port: " << port << std::endl;

		// Socket
		socket_fd = socket(domain, type, protocol);
		if (socket_fd < 0)
			throw (std::runtime_error(strerror(errno)));

		std::cout << "Socket created." << std::endl;
		
		// Bind settings
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);
		std::memset(address.sin_zero, 0, sizeof(address.sin_zero));

		// Bind
		struct sockaddr* addr_ptr = reinterpret_cast<struct sockaddr*>(&address);
		if (bind(socket_fd, addr_ptr, sizeof(struct sockaddr)) < 0)
		{
			close(socket_fd);
			throw (std::runtime_error(strerror(errno)));
		}

		std::cout << "Socket bound to address: "
			<< (address.sin_addr.s_addr & 0xFF000000) << '.'
			<< (address.sin_addr.s_addr & 0x00FF0000) << '.'
			<< (address.sin_addr.s_addr & 0x0000FF00) << '.'
			<< (address.sin_addr.s_addr & 0x000000FF)
			<< std::endl;

		// Listen
		const int max_queue = 10;
		if (listen(socket_fd, max_queue) < 0)
		{
			close(socket_fd);
			throw (std::runtime_error(strerror(errno)));
		}

		std::cout << "Server is now listening..." << std::endl;
	}

	bool receive_connection(void)
	{
		struct sockaddr accepted_address {0};
		socklen_t accepted_address_length = sizeof(struct sockaddr);
		int connection_fd = accept(socket_fd, &accepted_address, &accepted_address_length);
		if (connection_fd < 0)
			throw (std::runtime_error(strerror(errno)));
		
		std::cout << "Received connection: " << connection_fd << std::endl;
		connection(connection_fd, accepted_address, accepted_address_length);
		return (true);
	}

	~Server()
	{
		close(socket_fd);
	}

	private:
	int socket_fd;
	struct sockaddr_in address;

};

#endif // SERVER_H
