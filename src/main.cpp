#include "Connection.h"
#include "Server.h"
#include "Socket.h"
#include "Command.h"

#define TIMEOUT 1000

using namespace webserv;

std::vector<Server*> build_servers(void) // <- from JSON
{
	std::vector<Server*> servers;

	servers.push_back(new Server());
	return (servers);
}

std::vector<Socket*> build_sockets(int argc, char **argv) // <- from Servers
{
	std::vector<Socket*> sockets;

	for (int i = 1; i < argc; ++i)
		sockets.push_back(new Socket(std::atoi(argv[i])));
	return (sockets);
}

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
	// Json json = parse();

	if (argc < 2)
	{
		std::cout << "provide at least one port to use" << std::endl;
		return (0);
	}

	std::vector<Server*> servers = build_servers();
	std::vector<Socket*> sockets = build_sockets(argc, argv);
	std::unordered_map<sockfd_t, Socket*> fd_map = build_map(sockets);
	bool run = true;

	Command::add_command(
		Command("descriptors", [&]() {
			std::cout << "Descriptors in map: ";
			for (auto& pair : fd_map)
			{
				std::cout << pair.first << ", ";
			}
			std::cout << std::endl;
		})
	);

	Command::add_command(
		Command("exit", [&](){
			std::cout << "Stopping server..." << std::endl;
			run = false;
		})
	);

	terminal_setup();

	while (run)
	{
		std::vector<struct pollfd> fds = get_descriptors(fd_map);

		nfds_t data_size = static_cast<nfds_t>(fds.size());
		size_t amount = poll(fds.data(), data_size, TIMEOUT);

		if (amount < 0)
		{
			// error
			std::cerr << "poll() < 0: " << std::strerror(errno) << std::endl;
		}

		if (amount == 0)
			continue ;

		for (struct pollfd& pfd : fds)
		{
			if (pfd.revents != 0)
				fd_map[pfd.fd]->notify(pfd.fd, pfd.revents, fd_map);
		}
	}

	std::cout << "Bye!" << std::endl;

	return (EXIT_SUCCESS);
}
