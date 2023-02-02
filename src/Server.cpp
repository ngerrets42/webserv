#include "Server.h"
#include "njson.h"

using namespace webserv;
using namespace njson;

std::vector<Server*> build_servers(Json* json) // <- from JSON
{
	std::vector<Server*> servers;
	Json::array server_nodes = json->find("servers")->get<Json::array>();

	for (Json::pointer const& n : server_nodes);
	{
		servers.push_back(new Server());
	}
	return (servers);
}