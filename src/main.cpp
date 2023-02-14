#include "Server.h"
#include "Socket.h"
#include "Command.h"
#include "parsing.h"

#define TIMEOUT 1000

using namespace webserv;

static std::unordered_map<sockfd_t, Socket*> build_map(std::vector<std::unique_ptr<Socket>>& sockets)
{
	std::unordered_map<sockfd_t, Socket*> fd_map(sockets.size());

	for (auto& s : sockets)
		fd_map.insert({s->get_socket_fd(), s.get()});

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
		Connection const* c = pair.second->get_connection(pair.first);
		if (c && (c->get_state() == Connection::READY_TO_WRITE || c->get_state() == Connection::WRITING))
			tmp.events |= POLLOUT;
		fds.push_back(tmp);
	}
	return (fds);
}

int main(int argc, char **argv)
{
	njson::JsonParser json_parser("config/webserv.json");
	if (json_parser.has_error())
	{
		std::cerr << json_parser.get_error_msg() << std::endl;
		return (EXIT_FAILURE);
	}

	njson::Json::pointer root_node = json_parser.parse();
	if (!root_node)
	{
		std::cerr << "Invalid configuration file" << std::endl;
		return (EXIT_FAILURE);
	}

	std::vector<std::unique_ptr<Server>> servers = parse_servers(root_node);
	std::vector<std::unique_ptr<Socket>> sockets = build_sockets(servers);
	std::unordered_map<sockfd_t, Socket*> fd_map = build_map(sockets);

	bool run = true;
	command_init(fd_map, run);
	terminal_setup();

	while (run)
	{
		std::vector<struct pollfd> fds = get_descriptors(fd_map);

		size_t amount = poll(fds.data(), static_cast<nfds_t>(fds.size()), TIMEOUT);
		if (amount < 0)
			std::cerr << "poll() < 0: " << std::strerror(errno) << std::endl;
		if (amount == 0)
			continue ;

		for (struct pollfd& pfd : fds)
		{
			if (pfd.revents != 0)
				// Notify the connection/socket that a new event needs to be handled
				fd_map.at(pfd.fd)->notify(pfd.fd, pfd.revents, fd_map);
		}
	}

	std::cout << "Bye!" << std::endl;
	return (EXIT_SUCCESS);
}
