#include "Command.h"
#include "Socket.h"
#include "Connection.h"

namespace webserv {

// void command_init(pollable_map_t& fd_map, bool& run)
// {
// 	Command::add_command(
// 		new Command("help", [&](std::ostream& out, std::string str) {
// 			(void)str;
// 			out << "Commands:\n";
// 			for (auto& pair : Command::s_commands)
// 			{
// 				out << " - " << pair.first << "\n";
// 			}
// 			out << std::endl;
// 		})
// 	);

// 	Command::add_command(
// 		new Command("descriptors", [&](std::ostream& out, std::string str) {
// 			(void)str;
// 			out << "Descriptors in map: ";
// 			for (auto& pair : fd_map)
// 			{
// 				out << pair.first << ", ";
// 			}
// 			out << std::endl;
// 		})
// 	);

// 	Command::add_command(
// 		new Command("exit", [&](std::ostream& out, std::string str) {
// 			(void)str;
// 			out << "Stopping server..." << std::endl;
// 			run = false;
// 		})
// 	);

// 	Command::add_command(
// 		new Command("last_request", [&](std::ostream& out, std::string str) {
// 			sockfd_t fd;
// 			std::stringstream stream(str);
// 			stream >> fd;
// 			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
// 			{
// 				out << "descriptor does not exist" << std::endl;
// 				return ;
// 			}

// 			Connection const* connection = dynamic_cast<Connection*>(fd_map.at(fd));
// 			if (!connection)
// 			{
// 				out << "descriptor does not exist" << std::endl;
// 				return ;
// 			}

// 			out << "Connection " << fd << " last request:\n";
// 			request_print(connection->get_last_request(), out);
// 		})
// 	);

// 	Command::add_command(
// 		new Command("last_response", [&](std::ostream& out, std::string str) {
// 			sockfd_t fd;
// 			std::stringstream stream(str);
// 			stream >> fd;
// 			if (fd <= 0 || fd_map.find(fd) == fd_map.end())
// 			{
// 				out << "descriptor does not exist" << std::endl;
// 				return ;
// 			}

// 			Connection const* connection = dynamic_cast<Connection*>(fd_map.at(fd));
// 			if (!connection)
// 			{
// 				out << "descriptor does not exist" << std::endl;
// 				return ;
// 			}

// 			out << "Connection " << fd << " last response header:\n";
// 			out << connection->get_last_response().get_response_const() << std::endl;
// 		})
// 	);
// }

} // namespace webserv
