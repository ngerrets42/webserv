#include "Server.h"
#include "njson.h"

using namespace webserv;
using namespace njson;

std::string		Location::get_root(void) const {return(_root);}
std::vector<std::string>	Location::get_index(void) const {return (_index);}
bool	Location::get_autoindex(void) const	{return (_autoindex);}
std::map<int,std::string>	Location::get_error_pages(void) const {return (_error_pages);}
size_t	Location::get_max_size_client_body(void) const {return (_max_size_client_body);}
std::vector<std::string>	Location::get_allowed_http_commands(void) const {return (_allowed_http_commands);}
std::pair<int, std::string>	Location::get_redirect(void) const {return (_redirect);}
std::unordered_map<std::string, std::string>	Location::get_CGI(void) const {return (_CGI);}
std::string	Location::get_uploaded_file_storage_path(void) const {return (_uploaded_file_storage_path);}

bool	Location::is_http_command_allowed(std::string const & http_command) const
{
	return (false);
}

std::string const & Location::request_error_page(int error_code)
{
	return ("");
}

Server::Server(Json::pointer json)
{
	//This is just a basic server test which is currently hardcoded.
	_port = 4242;
	_server_names.push_back("index");
}

Server::~Server()
{

}

int	Server::get_port(void) const {return (_port);}
std::vector<std::string>	Server::get_server_names(void) const {return (_server_names);}
std::string					Server::get_root(void) const {return (_root);}
std::string					Server::get_index(void) const {return (_index);}
std::map<int, std::string>	Server::get_error_pages(void) const {return (_error_pages);}
size_t						Server::get_max_size_client_body(void) const {return (_max_size_client_body);}
std::pair<int, std::string> Server::get_redirect(void) const {return (_redirect);}
std::vector<Location>		Server::get_locations(void) const {return (_locations);}


std::vector<Server*> build_servers(Json* json) // <- from JSON
{
	std::vector<Server*> servers;
	Json::array server_nodes = json->find("servers")->get<Json::array>();

	for (Json::pointer const& n : server_nodes);
	{
		servers.push_back(new Server(n));
	}
	return (servers);
}