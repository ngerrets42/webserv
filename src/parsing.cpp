#include <set>
#include <cctype>
#include <algorithm>
#include <memory>
#include "parsing.h"
#include "ShellSocket.h"

namespace webserv {

static bool check_directives_location_block(njson::Json::object& loc){
	std::set<std::string>supported_directives({
		"client_body_size",
		"error_pages",
		"root",
		"locations",
		"allowed_methods",
		"index",
		"auto_index",
		"redirect"});

	njson::Json::object::iterator it;
	for(it = loc.begin(); it != loc.end(); ++it){
		if(supported_directives.count(it->first) == 0){
			std::cerr << "unknown directive '" << it->first << "' for location block" << std::endl;
			return false;
		}
	}
	return true;
}

static bool check_directives_server_block(njson::Json::object& serverblock){
	std::set<std::string>supported_directives({
		"listen",
		"host",
		"server_names",
		"client_body_size",
		"error_pages",
		"root",
		"locations",
		"allowed_methods",
		"index",
		"auto_index",
		"redirect"});

	njson::Json::object::iterator it;
	for(it = serverblock.begin(); it != serverblock.end(); ++it){
		if(supported_directives.count(it->first) == 0){
			std::cerr << "unknown directive '" << it->first << "' for server block" << std::endl;
			return false;
		}
	}
	return true;
}

//simple precheck of the ip address.
// It checks if string contains exactly 3 dots and digits
static bool	simple_ip_format_check(std::string const & ip){
	
	int count_dot = 0;

	for(size_t i = 0; i < ip.size(); ++i){
		if(ip[i] == '.' && count_dot < 3){
			count_dot++;
			continue;
		} else if (!std::isdigit(ip[i])){
			return false;
		}
	}
	if (count_dot != 3){
		return false;
	}
	return true;
}

static bool is_all_digits(std::string const & str){
	for(size_t i = 0; i < str.size(); ++i){
		if(!std::isdigit(str[i])){
			return false;
		}
	}
	return true;
}

static bool set_server_variables(njson::Json::object& serverblock, Server* server){
	//start setting the values of the serverblock
	//listen
	//if there is no listen directive in serverblock it will use default value of 80
	njson::Json::object::iterator it = serverblock.find("listen");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::INT){
			return false;
		}
		else{
			server->port = it->second->get<int>();
			if (server->port < 0){
				return false;
			}
		}
	}
	//host 
	//if there is no host directive in serverblock it will use the default value of 0.0.0.0
	it = serverblock.find("host");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			std::string host = it->second->get<std::string>();
			if (host.compare("localhost") == 0)
				host = "127.0.0.1";
			if(!simple_ip_format_check(host)){
				return false;
			}
			server->host = host;
		}
	} else {
		server->host = "0.0.0.0";
	}

	//server names
	it = serverblock.find("server_names");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			return false;
		} else {
			njson::Json::array& server_names = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < server_names.size(); ++i){
				if(server_names[i]->get_type() != njson::Json::STRING){
					return false;
				} else {
					server->add_server_name(server_names[i]->get<std::string>());
				}
			}
		}
	}

	//root
	//if not set it will default to the /var/www/html
	it = serverblock.find("root");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			server->root = it->second->get<std::string>();
		}
	}

	//index
	it = serverblock.find("index");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			server->index = it->second->get<std::string>();
		}
	}

	//autoindex
	it = serverblock.find("auto_index");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::BOOL){
			return false;
		} else {
			server->autoindex = it->second->get<bool>();
		}
	}

	//error_pages
	it = serverblock.find("error_pages");
	if (it != serverblock.end()){
		if(it->second->get_type() != njson::Json::OBJECT){
			return false;
		} else {
			njson::Json::object& error_pages = it->second->get<njson::Json::object>();
			njson::Json::object::iterator errit;
			for(errit = error_pages.begin(); errit != error_pages.end(); errit++){
				if (errit->second->get_type() != njson::Json::STRING){
					return false;
				} else {
					if(!is_all_digits(errit->first)){
						return false;
					}
					try{
						server->add_error_page(std::stoi(errit->first),errit->second->get<std::string>());
					} catch (std::exception e){
						return false;
					}
				}
			}
		}
	}

	//client_max_body_size
	//default size = 0 if not set which is for unlimited since the parser handles int the maximum value will be 2 Gb
	it = serverblock.find("client_body_size");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::INT){
			return false;
		} else {
			int body_size = it->second->get<int>();
			if(body_size < 0){
				return false;
			} else {
				server->client_max_body_size = body_size;
			}
		}
	}
	
	//redirect
	it = serverblock.find("redirect");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			server->redirect = it->second->get<std::string>();
		}
	}

	//allowed http commands
	it = serverblock.find("allowed_methods");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			return false;
		} else {
			njson::Json::array& allowed_http_commands = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < allowed_http_commands.size(); ++i){
				if(allowed_http_commands[i]->get_type() != njson::Json::STRING){
					return false;
				} else {
					server->add_allowed_http_command(allowed_http_commands[i]->get<std::string>());
				}
			}
		}
	}
	return true;
}

static bool set_location_variables(std::string const & path, njson::Json::object& locationblock, Location &loc){
	
	if (!check_directives_location_block(locationblock)){
		return false;
	}
	njson::Json::object::iterator it;
	loc.path = path;

	//root
	//if not set it will default to the server root value 
	it = locationblock.find("root");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			loc.root = it->second->get<std::string>();
		}
	}

	//index
	it = locationblock.find("index");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			loc.index = it->second->get<std::string>();
		}
	}
	
	//auto_index
	it = locationblock.find("auto_index");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::BOOL){
			return false;
		} else {
			loc.autoindex.first = true;
			loc.autoindex.second = it->second->get<bool>();
		}
	}

	//error_pages
	it = locationblock.find("error_pages");
	if (it != locationblock.end()){
		if(it->second->get_type() != njson::Json::OBJECT){
			return false;
		} else {
			njson::Json::object& error_pages = it->second->get<njson::Json::object>();
			njson::Json::object::iterator errit;
			for(errit = error_pages.begin(); errit != error_pages.end(); errit++){
				if (errit->second->get_type() != njson::Json::STRING){
					return false;
				} else {
					if(!is_all_digits(errit->first)){
						return false;
					}
					try{
						loc.add_error_page(std::stoi(errit->first),errit->second->get<std::string>());
					} catch (std::exception e){
						return false;
					}
				}
			}
		}
	}
	//client_max_body_size
	it = locationblock.find("client_body_size");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::INT){
			return false;
		} else {
			int body_size = it->second->get<int>();
			if(body_size < 0){
				return false;
			} else {
				loc.client_max_body_size.first = true;
				loc.client_max_body_size.second = body_size;
			}
		}
	}
	
	//redirect
	it = locationblock.find("redirect");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			return false;
		} else {
			loc.redirect = it->second->get<std::string>();
		}
	}
	//allowed http commands
	it = locationblock.find("allowed_methods");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			return false;
		} else {
			njson::Json::array& allowed_http_commands = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < allowed_http_commands.size(); ++i){
				if(allowed_http_commands[i]->get_type() != njson::Json::STRING){
					return false;
				} else {
					loc.add_allowed_http_command(allowed_http_commands[i]->get<std::string>());
				}
			}
		}
	}
	return true;
}

static bool process_locations(njson::Json::object& serverblock, Server* server){
	
	njson::Json::object::iterator it = serverblock.find("locations");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::OBJECT){
			return false;
		} else {
			njson::Json::object& locations = it->second->get<njson::Json::object>();
			njson::Json::object::iterator loc_it;
			for(loc_it = locations.begin(); loc_it != locations.end(); loc_it++){
				Location loc;
				if(!set_location_variables(loc_it->first, loc_it->second->get<njson::Json::object>(), loc)){
					return false;
				}
				server->add_location(loc);
			}
		}
	}
	return true;
}

Server* build_server(njson::Json::pointer& node)
{
	Server* server = new Server();

	//make sure that the servernode is the right type in this case an object aka unordered_map
	if (!node->is<njson::Json::object>())
	{
		std::cerr << "Not an object" << std::endl;
		return nullptr;
	}

	njson::Json::object& serverblock = node->get<njson::Json::object>();
	//make sure that all directives in server block are supported
	if(check_directives_server_block(serverblock) == false){
		return nullptr;
	}
	
	if(!set_server_variables(serverblock, server)){
		return nullptr;
	}
	if(!process_locations(serverblock, server)){
		return nullptr;
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
	
	return (servers);
}

std::vector<std::unique_ptr<Socket>> build_sockets(std::vector<std::unique_ptr<Server>>& servers)
{
	std::vector<std::unique_ptr<Socket>> sockets;
	std::set<int> ports_to_listen;


	for(size_t i = 0; i < servers.size(); ++i){
		int server_port = servers[i]->port;
		if(ports_to_listen.count(server_port) == 0){
			ports_to_listen.insert(server_port);
			sockets.emplace_back(new Socket(server_port));
		}
	}
//	sockets.emplace_back(new Socket(8080));

	//sockets.emplace_back(new ShellSocket(6666));
	return (sockets);
}

} // namespace webserv
