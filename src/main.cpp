#include "Connection.h"
#include "Core.h"
#include "Server.h"
#include "Socket.h"
#include "parsing.h"
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <sys/signal.h>
#include <ctime>
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

static std::vector<struct pollfd> get_descriptors(pollable_map_t& fd_map)
{
	std::vector<struct pollfd> fds;
	std::vector<sockfd_t> delete_vec;

	for (auto const& pair : fd_map)
	{
		// First check if this descriptor needs to be removed or not
		if (pair.second->should_destroy())
		{
			delete_vec.push_back(pair.first);
			continue ;
		}

		struct pollfd tmp = {};
		tmp.fd = pair.first;
		tmp.events = pair.second->get_events(pair.first);
		fds.push_back(tmp);
	}

	for (auto& fd : delete_vec)
	{
		auto it = fd_map.find(fd);
		if (it != fd_map.end())
		{
			delete it->second;
			fd_map.erase(fd);
		}
	}

	return (fds);
}

static bool s_run = true;
int main(int argc, char **argv)
{
	// TODO: Remove
	(void)std::atexit([]() { system("leaks -q webserv"); });

	(void)std::signal(SIGPIPE, [](int i) { (void)i; });

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

	// SIGINT needs to close the program cleanly
	(void)std::signal(SIGINT, [](int i) { (void)i; s_run = false; });

	while (s_run)
	{
		std::vector<struct pollfd> fds = get_descriptors(fd_map);

		size_t amount = poll(fds.data(), static_cast<nfds_t>(fds.size()), TIMEOUT);
		if (amount < 0)
			std::cerr << "poll() < 0: " << std::strerror(errno) << std::endl;

		for (struct pollfd& pfd : fds)
		{
			if (fd_map.find(pfd.fd) == fd_map.end())
				continue ;
			if (pfd.revents != 0)
			{
				// Notify the connection/socket/cgi that a new event needs to be handled
				fd_map.at(pfd.fd)->notify(pfd.revents, fd_map, pfd.fd);
			}
			else fd_map.at(pfd.fd)->on_post_poll(fd_map);
		}
	}

	std::cout << "losing webserv^" << std::endl;
	std::cout << "\n\n === PLEASE WAIT ===\n\n" << std::endl;

	// Remove sockets from fd_map and close them
	for (auto& uptr : sockets)
	{
		close(uptr->get_fd());
		fd_map.erase(uptr->get_fd());
	}

	// This loop keeps running until ALL Pollable objects are destroyed
	// So it can free and close the program properly
	while (!fd_map.empty())
	{
		std::vector<sockfd_t> destroy_vec;
		for (auto& pair : fd_map)
		{
			pair.second->notify(POLLHUP, fd_map, pair.first);
			if (pair.second->should_destroy())
				destroy_vec.push_back(pair.first);
		}
		for (auto& fd : destroy_vec)
		{
			delete fd_map.at(fd);
			fd_map.erase(fd);
		}
	}

	std::cout << "Bye!" << std::endl;

	return (EXIT_SUCCESS);
}
