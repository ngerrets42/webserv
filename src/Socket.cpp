#include "Socket.h"
#include "Connection.h"
#include "Core.h"
#include <arpa/inet.h>
#include <memory>
#include <stdexcept>
#include <sys/socket.h>

namespace webserv {

Socket::Socket(uint16_t _port, std::string const& _host) : port(_port), host(_host)
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

	//set the value of SO_REUSEADDR to true (1);
	int const optval = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		throw (std::runtime_error(std::strerror(errno)));

	// Bind settings
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(host.c_str());
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
	if (listen(socket_fd, MAX_SOCKET_QUEUE) < 0)
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

Socket::~Socket()
{
	std::cout << "Socket " << socket_fd << " destroyed." << std::endl;
	close(socket_fd);
}

// Unavailable constructors
Socket::Socket() : socket_fd(-1) {};
Socket::Socket(Socket const& other) { (void)other; }
Socket& Socket::operator=(Socket const& other) { (void)other; return *this; }

// POLLING
void Socket::on_pollin(pollable_map_t& fd_map)
{
	accept_connections(fd_map);
}

void Socket::on_pollout(pollable_map_t& fd_map) { (void)fd_map; }

short Socket::get_events(sockfd_t fd) const
{
	(void)fd;
	return (POLLIN); // Sockets only really respond to POLLIN as they need to accept connections
}

uint16_t Socket::get_port(void) const { return port; }
std::string const& Socket::get_host(void) const { return host; }

void Socket::accept_connections(pollable_map_t& fd_map)
{
	// new CONNECTIONs are coming in
	addr_t accepted_address = {};
	socklen_t accepted_address_length = sizeof(addr_t);
	sockfd_t connection_fd = 0;

	std::cout << "Socket (" << socket_fd << ")\n";
	while ((connection_fd = accept(socket_fd, &accepted_address, &accepted_address_length)) > 0)
	{
		Connection* c = new Connection(connection_fd, accepted_address, this);
		std::cout << "- accepted: " << connection_fd << ", ip: " << c->get_ip() << '\n';

		// Add to the fd_map for poll()
		fd_map.emplace(connection_fd, c);
	}
	std::cout << std::endl;
}

Server& Socket::get_server(std::string const& host)
{
	if (servers.empty())
		throw (std::runtime_error("Socket has no servers"));
	std::string hostname = host;
	size_t pos = hostname.find_last_of(':');
	if (pos != std::string::npos)
		hostname = hostname.substr(0, pos);
	for (auto* s : servers)
	{
		if (s->contain_server_name(hostname))
			return (*s);
	}
	return (*servers[0]);
}

void Socket::add_server_ref(std::unique_ptr<Server>& server_ref)
{
	servers.push_back(server_ref.get());
}

sockfd_t Socket::get_fd(void) const { return socket_fd; }

bool Socket::should_destroy(void) const { return false; }


} // namespace webserv
