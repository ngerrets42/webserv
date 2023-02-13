#include "Connection.h"
#include "Server.h"
#include "Socket.h"
#include "Command.h"
#include "ShellSocket.h"

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
	sockets.push_back(new ShellSocket(6666));
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
		new Command("help", [&](std::ostream& out, std::string str) {
			out << "Commands:\n";
			for (auto& pair : Command::s_commands)
			{
				out << " - " << pair.first << "\n";
			}
			out << std::endl;
		})
	);

	Command::add_command(
		new Command("descriptors", [&](std::ostream& out, std::string str) {
			out << "Descriptors in map: ";
			for (auto& pair : fd_map)
			{
				out << pair.first << ", ";
			}
			out << std::endl;
		})
	);

	Command::add_command(
		new Command("exit", [&](std::ostream& out, std::string str) {
			out << "Stopping server..." << std::endl;
			run = false;
		})
	);

	Command::add_command(
		new Command("echo", [](std::ostream& out, std::string str) {
			out << str << std::endl;
		})
	);

	Command* cmd = new Command("socket");
	cmd->add_subcommand(new Command("status", [&](std::ostream& out, std::string str) {
			sockfd_t fd = -1;
			std::stringstream stream(str);
			stream >> fd;
			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			Socket const* socket = fd_map.at(fd);

			out << "Socket " << fd << '\n';
			out << " Port: " << socket->get_port() << std::endl;
		}));
	Command::add_command(cmd);

	cmd = new Command("connection");
	cmd->add_subcommand(new Command("status", [&](std::ostream& out, std::string str) {
			sockfd_t fd;
			std::stringstream stream(str);
			stream >> fd;
			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			Socket const* socket = fd_map.at(fd);
			Connection const* connection = socket->get_connection(fd);
			if (!connection)
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			out << "Connection " << fd << '\n';
			out << " Parent Socket: " << socket->get_socket_fd() << '\n';
			out << " Busy: " << std::boolalpha << connection->busy << '\n';
			out << " IP: " << connection->get_ip() << std::endl;
		}));
	Command::add_command(cmd);

	Command::add_command(
		new Command("last_request", [&](std::ostream& out, std::string str) {
			sockfd_t fd;
			std::stringstream stream(str);
			stream >> fd;
			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			Socket const* socket = fd_map.at(fd);
			Connection const* connection = socket->get_connection(fd);
			if (!connection)
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			out << "Connection " << fd << " last request:\n";
			request_print(connection->get_last_request(), out);
		})
	);

	Command::add_command(
		new Command("last_response", [&](std::ostream& out, std::string str) {
			sockfd_t fd;
			std::stringstream stream(str);
			stream >> fd;
			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			Socket const* socket = fd_map.at(fd);
			Connection const* connection = socket->get_connection(fd);
			if (!connection)
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			out << "Connection " << fd << " last response header:\n";
			out << connection->get_last_response().header << std::endl;
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
