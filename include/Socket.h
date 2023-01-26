#ifndef SOCKET_H
# define SOCKET_H

# include "Core.h"

# include "Server.h"
# include "Connection.h"

namespace webserv {

class Socket
{

	private:
	sockfd_t socket_fd;
	std::vector<Server> servers;
	std::unordered_map<sockfd_t, Connection> connecton_map; // map of Connection socket fd - Connection class
};

} // webserv

#endif // SOCKET_H
