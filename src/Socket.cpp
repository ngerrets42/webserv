#include "Socket.h"

# include "RequestHandler.h"

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
		throw (std::runtime_error(std::strerror(errno)));

	// Set non-blocking
	if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) < 0)
		throw (std::runtime_error(std::strerror(errno)));

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
		throw (std::runtime_error(std::strerror(errno)));
	}

	// Listen
	const int max_queue = 10;
	if (listen(socket_fd, max_queue) < 0)
	{
		close(socket_fd);
		throw (std::runtime_error(std::strerror(errno)));
	}

	// Info
	std::cout << "Socket Created {address: "
		<< inet_ntoa(address.sin_addr) << ":" << port
		<< ", fd: " << socket_fd << '}'
		<< std::endl;
}

Socket::~Socket() { close(socket_fd); }

// Unavailable constructors
Socket::Socket() : socket_fd(-1) {};
Socket::Socket(Socket const& other) { (void)other; }
Socket& Socket::operator=(Socket const& other) { (void)other; return *this; }


void Socket::notify(sockfd_t fd, short revents, std::unordered_map<sockfd_t, Socket*>& fd_map)
{
	std::cout << "Notify {fd: " << fd << ", socket: " << socket_fd << "}, ";

	if (fd == socket_fd)
	{
		if (revents & POLLERR)
		{
			// error on socket??
			std::cout << "event: POLLERR" << std::endl;
			throw (std::runtime_error(std::string("POLLERR {socket:" + std::to_string(socket_fd) + "}")));
		}
		else if (revents & POLLIN)
		{
			std::cout << "event: POLLIN";
			accept_connections(fd_map);
		}
		return ;
	}

	auto it = connection_map.find(fd);
	if (it == connection_map.end())
	{
		// unknown fd, shouldn't happen
		throw (std::runtime_error(std::string("sockfd not in connection_map {socket:" + std::to_string(socket_fd) + "}")));
	}

	if (revents & POLLERR)
	{
		// received ERR, destroy connection
		std::cout << "event: POLLERR" << std::endl;
		delete it->second;
		connection_map.erase(it);
		// update fd_map
		fd_map.erase(fd);
		return ;
	}
	if (revents & POLLHUP)
	{
		std::cout << "event: POLLHUP" << std::endl;
		delete it->second;
		connection_map.erase(it);
		fd_map.erase(fd);
		return ;
	}
	else if (revents & POLLIN)
	{
		// A new REQUEST is coming in on this connection
		std::cout << "event: POLLIN";
		if (it->second->busy)
			std::cout << " - ignored because connection busy";
		else
			RequestHandler::async(this, it->second, it->first);
		std::cout << std::endl;
	}
}

void Socket::accept_connections(std::unordered_map<sockfd_t, Socket*>& fd_map)
{
	// new CONNECTIONs are coming in
	addr_t accepted_address = {};
	socklen_t accepted_address_length = sizeof(addr_t);
	sockfd_t connection_fd = 0;
	while ((connection_fd = accept(socket_fd, &accepted_address, &accepted_address_length)) > 0)
	{
		std::cout << ", accepted: " << connection_fd;
		Connection* c = new Connection(connection_fd, accepted_address);

		// Add to connections AND to the fd_map for poll()
		connection_map.emplace(connection_fd, c);
		fd_map.emplace(connection_fd, this);
	}
	std::cout << std::endl;
}

sockfd_t Socket::get_socket_fd(void) { return socket_fd; }


} // webserv
