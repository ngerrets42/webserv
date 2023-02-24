#include "Socket.h"

#include <memory>
#include <stdexcept>

namespace webserv {

Socket::Socket(uint16_t _port) : port(_port)
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
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
	const int max_queue = 100;
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

Socket::~Socket()
{
	for (auto& pair : connection_map)
		delete pair.second;
	close(socket_fd);
}

// Unavailable constructors
Socket::Socket() : socket_fd(-1) {};
Socket::Socket(Socket const& other) { (void)other; }
Socket& Socket::operator=(Socket const& other) { (void)other; return *this; }

void Socket::notify(sockfd_t fd, short revents, std::unordered_map<sockfd_t, Socket*>& fd_map)
{
	std::cout << "Notify {fd: " << fd << ", socket: " << socket_fd << "}, ";

	if (fd == socket_fd)
	{
		if ((revents & POLLERR) != 0)
		{
			// error on socket??
			std::cout << "event: POLLERR" << std::endl;
			throw (std::runtime_error(std::string("POLLERR {socket:" + std::to_string(socket_fd) + "}")));
		}
		if ((revents & POLLIN) != 0)
		{
			std::cout << "event: POLLIN";
			accept_connections(fd_map);
		}
		return ;
	}

	auto it = connection_map.find(fd);
	if (it == connection_map.end())
	{
		// unknown fd
		fd_map.erase(fd);
		// throw (std::runtime_error(std::string("sockfd not in connection_map {socket:" + std::to_string(socket_fd) + "}")));
	}

	if ((revents & POLLERR) != 0)
	{
		// received ERR, destroy connection
		std::cout << "event: POLLERR" << std::endl;
		delete it->second;
		connection_map.erase(it);
		// update fd_map
		fd_map.erase(fd);
		return ;
	}
	if ((revents & POLLHUP) != 0)
	{
		std::cout << "event: POLLHUP" << std::endl;
		delete it->second;
		connection_map.erase(it);
		fd_map.erase(fd);
		return ;
	}
	if ((revents & POLLIN) != 0)
		on_pollin(it->first, it->second); // A new REQUEST is coming in on this connection
	else if ((revents & POLLOUT) != 0)
		on_pollout(it->first, it->second);
	
	// Close connections that need to close
	if (it->second->get_state() == Connection::CLOSE)
	{
		delete it->second;
		connection_map.erase(it);
		fd_map.erase(fd);
	}
}

void Socket::on_pollin(sockfd_t fd, Connection* connection)
{
	(void)fd;
	std::cout << "event: POLLIN";
	connection->on_pollin(*this);
	std::cout << std::endl;
}

void Socket::on_pollout(sockfd_t fd, Connection* connection)
{
	(void)fd;
	std::cout << "event: POLLOUT";
	connection->on_pollout(*this);
	std::cout << std::endl;
}

Connection const* Socket::get_connection(sockfd_t fd) const
{
	auto it = connection_map.find(fd);
	if (it == connection_map.end())
		return (nullptr);
	return (it->second);
}

uint16_t Socket::get_port(void) const
{
	return port;
}

bool Socket::is_active(sockfd_t fd) const
{
	if (fd == socket_fd)
		return (true);
	auto it = connection_map.find(fd);
	if (it == connection_map.end())
		return (false);
	return (!connection_map.at(fd)->busy);
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

Server& Socket::get_server(std::string const& host)
{
	if (servers.empty())
		throw (std::runtime_error("Socket has no servers"));
	for (auto* s : servers)
	{
		if (s->contain_server_name(host))
			return (*s);
	}
	return (*servers[0]);
}

void Socket::add_server_ref(std::unique_ptr<Server>& server_ref)
{
	servers.push_back(server_ref.get());
}

sockfd_t Socket::get_socket_fd(void) const { return socket_fd; }


} // namespace webserv
