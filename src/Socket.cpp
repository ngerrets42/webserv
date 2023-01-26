#include "Socket.h"

namespace webserv {

Socket::Socket(uint16_t port)
{
	// Settings
	const int domain = AF_INET;
	const int type = SOCK_STREAM;
	const int protocol = 0;

	// Socket
	socket_fd = socket(domain, type, protocol);
	if (socket_fd < 0)
		throw (std::runtime_error(strerror(errno)));

	// Set non-blocking
	if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) < 0)
		throw (std::runtime_error(strerror(errno)));

	// Bind settings
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	std::memset(address.sin_zero, 0, sizeof(address.sin_zero));

	// Bind
	addr_t* addr_ptr = reinterpret_cast<addr_t*>(&address);
	if (bind(socket_fd, addr_ptr, sizeof(addr_t)) < 0)
	{
		close(socket_fd);
		throw (std::runtime_error(strerror(errno)));
	}

	// Listen
	const int max_queue = 10;
	if (listen(socket_fd, max_queue) < 0)
	{
		close(socket_fd);
		throw (std::runtime_error(strerror(errno)));
	}

	// Info
	std::cout << "Socket Created {address: " << inet_ntoa(address.sin_addr) << ":" << port << '}' << std::endl;
}

Socket::~Socket() { close(socket_fd); }

// Unavailable constructors
Socket::Socket() : socket_fd(-1) {};
Socket::Socket(Socket const& other) { (void)other; }
Socket& Socket::operator=(Socket const& other) { (void)other; return *this; }


void Socket::notify(sockfd_t fd, short revents, std::unordered_map<sockfd_t, Socket>& fd_map)
{
	if (fd == socket_fd)
	{
		if (revents & POLL_ERR)
		{
			// error on socket??
			throw (std::runtime_error(std::string("POLL_ERR {socket:" + std::to_string(socket_fd) + "}")));
		}
		else if (revents & POLL_IN)
			accept_connections(fd_map);
		return ;
	}

	auto it = connection_map.find(fd);
	if (it == connection_map.end())
	{
		// unknown fd, shouldn't happen
		throw (std::runtime_error(std::string("sockfd not in connection_map {socket:" + std::to_string(socket_fd) + "}")));
	}

	if (revents & POLL_ERR || revents & POLL_HUP)
	{
		// received ERR or CLOSE, destroy connection
		connection_map.erase(it);
		// update fd_map
		fd_map.erase(fd);
		return ;
	}
	else if (revents & POLL_IN)
	{
		// A new REQUEST is coming in on this connection
		Request request = it->second->receive_request();
		Response response = build_response( request );
		it->second->send_response( response );
	}
}

Response Socket::build_response(Request& request)
{
	Response response;

	response.header = "";

	return (response);
}

void Socket::accept_connections(std::unordered_map<sockfd_t, Socket>& fd_map)
{
	// new CONNECTIONs are coming in
	addr_t accepted_address {0};
	socklen_t accepted_address_length = sizeof(addr_t);
	sockfd_t connection_fd = 0;
	while ((connection_fd = accept(socket_fd, &accepted_address, &accepted_address_length)) > 0)
	{
		std::cout << "incoming connection: " << connection_fd << std::endl;
		Connection* c = new Connection(connection_fd, accepted_address);
		Request request = c->receive_request();
		Response response = build_response( request );
		c->send_response( response );
		connection_map.emplace(connection_fd, c);
	}
}


} // webserv
