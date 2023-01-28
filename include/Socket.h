#ifndef SOCKET_H
# define SOCKET_H

# include "Core.h"

# include "Server.h"
# include "Connection.h"

namespace webserv {

class Socket
{
	public:
	// Constructors
	Socket(uint16_t port);

	~Socket();

	private:
	// Private Constructors
	Socket();
	Socket(Socket const& other);
	Socket& operator=(Socket const& other);

	public:
	void notify(sockfd_t fd, short revents, std::unordered_map<sockfd_t, Socket*>& fd_map);

	Response build_response(Request& request);

	std::vector<Server*> get_servers(void);

	sockfd_t get_socket_fd(void);

	private:
	void accept_connections(std::unordered_map<sockfd_t, Socket*>& fd_map);

	void build_response_get(Request& request, Response& response);
	void build_response_post(Request& request, Response& response);

	private:
	sockfd_t socket_fd;
	addr_in_t address;

	std::vector<Server*> servers;
	std::unordered_map<sockfd_t, Connection*> connection_map; // map of Connection socket fd - Connection class
};

} // webserv

#endif // SOCKET_H
