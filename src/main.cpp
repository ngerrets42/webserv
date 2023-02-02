#include "Connection.h"
#include "Server.h"
#include "Socket.h"

#define TIMEOUT 10000

using namespace webserv;
using namespace njson;


std::unordered_map<sockfd_t, Socket*> build_map(std::vector<Socket*>& sockets)
{
	std::unordered_map<sockfd_t, Socket*> fd_map;

	for (auto* s : sockets)
		fd_map.insert({s->get_socket_fd(), s});

	return (fd_map);
}

std::vector<struct pollfd> get_descriptors(std::unordered_map<sockfd_t, Socket*> fd_map)
{
	std::vector<struct pollfd> fds;

	for (auto& pair : fd_map)
	{
		if (!pair.second->is_active(pair.first)) // To make sure only relevant connections are polled (connections that are NOT busy)
			continue ;
		struct pollfd tmp = {};
		tmp.fd = pair.first;
		tmp.events = POLLHUP | POLLIN;
		fds.push_back(tmp);
	}
	return (fds);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cout << "Please provide a .json configuration file as an arguement" << std::endl;
		return (EXIT_FAILURE);
	}
	if (argc > 2)
	{
		std::cout << "Please provide only a .json configuration file as an arguement" << std::endl;
		return (EXIT_FAILURE);
	}
	JsonParser parser(argv[1]);
	Json::pointer json = parser.parse();
	if (parser.has_error())
	{
		std::cout << parser.get_error_msg() << std::endl;
		return (EXIT_FAILURE);
	}
	if (!json)
	{
		std::cout << "Json Node was a nullptr" << std::endl;
		return (EXIT_FAILURE);
	}
	std::vector<Server*> servers = build_servers(json);
	// std::vector<Socket*> sockets = build_sockets(argc, argv);
	// std::unordered_map<sockfd_t, Socket*> fd_map = build_map(sockets);

	return (EXIT_SUCCESS);
}
