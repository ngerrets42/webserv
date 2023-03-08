#include "Core.h"
#include "Server.h"
#include "Command.h"
#include "Socket.h"
#include "parsing.h"
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <sys/signal.h>
#include <unordered_map>

#define TIMEOUT 1000

using namespace webserv;

static pollable_map_t build_map(std::vector<std::unique_ptr<Socket>>& sockets)
{
	pollable_map_t fd_map(sockets.size());

	for (auto& s : sockets)
		fd_map.insert({s->get_fd(), s.get()});

	return (fd_map);
}

std::vector<struct pollfd> get_descriptors(pollable_map_t const& fd_map)
{
	std::vector<struct pollfd> fds;

	for (auto const& pair : fd_map)
	{
		struct pollfd tmp = {};
		tmp.fd = pair.first;
		tmp.events = pair.second->get_events(pair.first);
		fds.push_back(tmp);
	}

	return (fds);
}

#include <csignal>

int main(int argc, char **argv)
{
	std::signal(SIGINT, [](int i){
		std::cout << i << " SIGINT" << std::endl;
		exit(1);
	});

	std::signal(SIGHUP, [](int i){
		std::cout << i << " SIGHUP" << std::endl;
		exit(1);
	});

	std::signal(SIGABRT, [](int i){
		std::cout << i << " SIGABRT" << std::endl;
		exit(1);
	});

	std::signal(SIGPIPE, [](int i){
		std::cout << i << " SIGPIPE" << std::endl;
	});

	std::signal(SIGTERM, [](int i){
		std::cout << i << " SIGTERM" << std::endl;
		exit(1);
	});

	std::signal(SIGIO, [](int i){
		std::cout << i << " SIGIO" << std::endl;
		exit(1);
	});

	std::signal(SIGURG, [](int i){
		std::cout << i << " SIGURG" << std::endl;
		exit(1);
	});

	std::signal(SIGTRAP, [](int i){
		std::cout << i << " SIGTRAP" << std::endl;
		exit(1);
	});

	std::signal(SIGKILL, [](int i){
		std::cout << i << " SIGKILL" << std::endl;
		exit(1);
	});

	std::signal(SIGILL, [](int i){
		std::cout << i << " SIGILL" << std::endl;
		exit(1);
	});

	std::signal(SIGQUIT, [](int i){
		std::cout << i << " SIGQUIT" << std::endl;
		exit(1);
	});

	std::atexit([](){
		system("leaks -q webserv");
	});

	std::string config_path = "config/webserv.json";

	if (argc > 1)
		config_path = argv[1];

	njson::JsonParser json_parser(config_path);
	if (json_parser.has_error())
	{
		std::cerr << "Can't open file: " << config_path << std::endl;
		return (EXIT_FAILURE);
	}

	njson::Json::pointer root_node = json_parser.parse();
	if (root_node->is<std::nullptr_t>())
	{
		std::cerr << "Invalid configuration file" << std::endl;
		return (EXIT_FAILURE);
	}

	std::vector<std::unique_ptr<Server>> servers = parse_servers(root_node);
	if (servers.empty())
		return (EXIT_FAILURE);

	std::vector<std::unique_ptr<Socket>> sockets = build_sockets(servers);
	pollable_map_t fd_map = build_map(sockets);

	bool run = true;
	// command_init(fd_map, run);

	while (run)
	{
		std::vector<struct pollfd> fds = get_descriptors(fd_map);

		size_t amount = poll(fds.data(), static_cast<nfds_t>(fds.size()), TIMEOUT);
		if (amount < 0)
			std::cerr << "poll() < 0: " << std::strerror(errno) << std::endl;
		if (amount == 0) continue ;

		for (struct pollfd& pfd : fds)
		{
			if (pfd.revents != 0)
				// Notify the connection/socket that a new event needs to be handled
				fd_map.at(pfd.fd)->notify(pfd.revents, fd_map);
		}
	}

	std::cout << "Bye!" << std::endl;
	return (EXIT_SUCCESS);
}
