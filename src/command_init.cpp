#include "Command.h"
#include "Socket.h"

namespace webserv {

void command_init(std::unordered_map<sockfd_t, Socket*>& fd_map, bool& run)
{
	Command::add_command(
		new Command("help", [&](std::ostream& out, std::string str) {
			(void)str;
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
			(void)str;
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
			(void)str;
			out << "Stopping server..." << std::endl;
			run = false;
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

			auto const& socket = fd_map.at(fd);

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

			auto const& socket = fd_map.at(fd);
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

			auto const& socket = fd_map.at(fd);
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

			auto const& socket = fd_map.at(fd);
			Connection const* connection = socket->get_connection(fd);
			if (!connection)
			{
				out << "descriptor does not exist" << std::endl;
				return ;
			}

			out << "Connection " << fd << " last response header:\n";
			out << connection->get_last_response().get_response_const() << std::endl;
		})
	);
}

} // webserv
