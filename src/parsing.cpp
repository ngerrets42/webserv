#include "parsing.h"
#include "ShellSocket.h"
#include <_types/_uint16_t.h>

namespace webserv {

static void build_errorpages(Server* server, njson::Json::pointer& node)
{
	for (auto& pair : node->get<njson::Json::object>())
	{
		server->add_error_page(std::stoi(pair.first), pair.second->get<std::string>());
	}
}


Server* build_server(njson::Json::pointer& node)
{

	Server* server = new Server();

	if (!node->is<njson::Json::object>())
	{
		std::cerr << "Not an object" << std::endl;
		return nullptr;
	}

	try
	{
		uint16_t port = node->find("listen")->get<int>();
		std::string host = node->find("host")->get<std::string>();

		build_errorpages(server, node->find("error_pages"));

	}
	catch (njson::Json::json_exception& e)
	{
		std::cerr << e.what() << std::endl;
		delete server;
		return (nullptr);
	}

	return (new Server());
}

// allocates new servers
std::vector<std::unique_ptr<Server>> parse_servers(njson::Json::pointer& root_node)
{
	std::vector<std::unique_ptr<Server>> servers;

	njson::Json::pointer& servers_node = root_node->find("servers");
	if (!servers_node || servers_node->get_type() != njson::Json::ARRAY)
	{
		std::cerr << "\"servers\" node (array) required. Please check your configuration file." << std::endl;
		return (servers);
	}

	njson::Json::array& server_array = servers_node->get<njson::Json::array>();
	for (auto& node : server_array)
	{
		Server* tmp = build_server(node);
		if (tmp == nullptr)
			return (std::vector<std::unique_ptr<Server>> {});
		servers.emplace_back(tmp);
	}

	return (servers);
}

std::vector<std::unique_ptr<Socket>> build_sockets(std::vector<std::unique_ptr<Server>>& servers)
{
	std::vector<std::unique_ptr<Socket>> sockets;

	sockets.emplace_back(new Socket(8080));
	sockets.emplace_back(new ShellSocket(6666));
	return (sockets);
}

} // namespace webserv
