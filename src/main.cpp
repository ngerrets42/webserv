#include <vector>
#include <poll.h>
#include <unordered_map>
#include <fstream>

#include "Connection.h"

#define TIMEOUT 10000

class Server {}; // holds server configuration (locations)
class Socket {}; // holds sockets, servers and connections.

std::vector<Server> build_servers(void) // <- from JSON
{
	std::vector<Server> servers;

	servers.push_back(Server());
	return (servers);
}

std::vector<Socket> build_sockets(void) // <- from JSON
{
	std::vector<Socket> sockets;

	sockets.push_back(Socket());
	return (sockets);
}

std::unordered_map<int, Socket&> build_map(std::vector<Socket>& sockets)
{
	std::unordered_map<int, Socket&> fd_map;

	fd_map.insert({3, sockets[0]});

	return (fd_map);
}

std::vector<struct pollfd> get_descriptors(std::vector<Socket>& sockets)
{
	std::vector<struct pollfd> fds;

	struct pollfd tmp;
	tmp.events = POLL_HUP | POLL_IN;

	fds.push_back(tmp);

	return (fds);
}

// int main(void)
// {
// 	// Json json = parse();

// 	std::vector<Server> servers = get_servers();
// 	std::unordered_map<int, Server&> fd_map = build_map(servers);

// 	while (true)
// 	{
// 		std::vector<struct pollfd> fds = get_descriptors(servers);

// 		nfds_t data_size = static_cast<nfds_t>(fds.size());
// 		size_t amount = poll(fds.data(), data_size, TIMEOUT);

// 		if (amount < 0)
// 		{
// 			// error
// 		}

// 		if (amount == 0)
// 			continue ;

// 		for (struct pollfd& pfd : fds)
// 		{
// 			// if (pfd.revents != 0)
// 			// 	fd_map[pfd.fd].notify(pfd.fd, pfd.revents, fd_map);
		
// 		}
// 	}

// 	return (EXIT_SUCCESS);
// }

int main(void)
{
	using namespace webserv;

	Connection test(3, sockaddr {});

	Request r = test.build_request("GET / HTTP/1.1\nHost: test.com");

	request_print(r);

	return (EXIT_SUCCESS);
}