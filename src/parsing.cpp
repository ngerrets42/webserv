#include "parsing.h"
#include "ShellSocket.h"
#include <algorithm>
#include <memory>

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
		// uint16_t port = node->find("listen")->get<int>();
		// std::string host = node->find("host")->get<std::string>();

		// build_errorpages(server, node->find("error_pages"));

	}
	catch (njson::Json::json_exception& e)
	{
		std::cerr << "json_exception: " << e.what() << std::endl;
		delete server;
		return (nullptr);
	}

	return (server);
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

	// CUSTOM SERVER
	// TODO: remove
	Server* s = new Server();

	s->port = 8080;
	s->add_server_name("localhost");
	s->add_error_page(404, "404.html");
	s->add_allowed_http_command("GET");
	s->root = "var/www/html";
	s->host = "0.0.0.0";

	Location loc1("/");
	loc1.index = "index.html";

	Location loc2("/pages/");
	loc2.add_allowed_http_command("GET");
	loc2.set_auto_index(true);
	loc2.index = "";

	s->add_location(loc1);
	s->add_location(loc2);

	servers.emplace_back(s);

	return (servers);
}

std::vector<std::unique_ptr<Socket>> build_sockets(std::vector<std::unique_ptr<Server>>& servers)
{
	std::vector<std::unique_ptr<Socket>> sockets;

	sockets.emplace_back(new ShellSocket(6666));

	for (auto& s : servers)
	{
		auto it = std::find_if(sockets.begin(), sockets.end(), [&](std::unique_ptr<Socket>& sock){
			return (sock->get_port() == s->port);
		});

		if (it != sockets.end())
			it->get()->add_server_ref(s);
		else
		{
			Socket* sock = new Socket(s->port);
			sock->add_server_ref(s);
			sockets.emplace_back(sock);
		}
	}

	return (sockets);
}

} // namespace webserv
