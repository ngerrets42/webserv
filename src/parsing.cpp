#include "parsing.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <set>

namespace webserv {

static void	print_error(std::string const & message){
	std::cerr << "Error parsing config file : " << message << std::endl;
}

static bool check_directives_location_block(njson::Json::object& loc){
	std::set<std::string>supported_directives({
		"client_body_size",
		"error_pages",
		"root",
		"locations",
		"allowed_methods",
		"index",
		"auto_index",
		"redirect",
		"CGI",
		"upload_directory"});

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
			print_error("listen value is not an integer");
			return false;
		}
		else{
			server->port = it->second->get<int>();
			if (server->port < 0){
				print_error("listen value can't be negative");
				return false;
			}
		}
	}
	//host 
	//if there is no host directive in serverblock it will use the default value of 0.0.0.0
	it = serverblock.find("host");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			print_error("host value needs to be a string");
			return false;
		} else {
			std::string host = it->second->get<std::string>();
			if (host == "localhost")
				host = "127.0.0.1";
			if(!simple_ip_format_check(host)){
				print_error("host value is not an IPv4 address");
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
			print_error("server_names needs to be set in an array");
			return false;
		} else {
			njson::Json::array& server_names = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < server_names.size(); ++i){
				if(server_names[i]->get_type() != njson::Json::STRING){
					print_error("server_names values needs to be string");
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
			print_error("root value needs to be a string");
			return false;
		} else {
			server->root = it->second->get<std::string>();
		}
	}

	//index
	it = serverblock.find("index");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			print_error("index value needs to be a string");
			return false;
		} else {
			server->index = it->second->get<std::string>();
		}
	}

	//autoindex
	it = serverblock.find("auto_index");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::BOOL){
			print_error("auto_index value needs to be a boolean");
			return false;
		} else {
			server->autoindex = it->second->get<bool>();
		}
	}

	//error_pages
	it = serverblock.find("error_pages");
	if (it != serverblock.end()){
		if(it->second->get_type() != njson::Json::OBJECT){
			print_error("error_pages needs to be in the key pair format of two strings");
			return false;
		} else {
			njson::Json::object& error_pages = it->second->get<njson::Json::object>();
			njson::Json::object::iterator errit;
			for(errit = error_pages.begin(); errit != error_pages.end(); errit++){
				if (errit->second->get_type() != njson::Json::STRING){
					print_error("error_page value needs to be a string");
					return false;
				} else {
					if(!is_all_digits(errit->first)){
						print_error("error page key value needs to be all digits");
						return false;
					}
					try{
						server->add_error_page(std::stoi(errit->first),errit->second->get<std::string>());
					} catch (std::exception e){
						print_error("couldn't process the error code");
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
			print_error("client_body_size value needs to be an integer");
			return false;
		} else {
			int body_size = it->second->get<int>();
			if(body_size < 0){
				print_error("client_body_size value can not be negative");
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
			print_error("redirect value need to be a string");
			return false;
		} else {
			server->redirect = it->second->get<std::string>();
		}
	}

	//allowed http commands
	it = serverblock.find("allowed_methods");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			print_error("allowed_methods needs to be set in an array");
			return false;
		} else {
			njson::Json::array& allowed_http_commands = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < allowed_http_commands.size(); ++i){
				if(allowed_http_commands[i]->get_type() != njson::Json::STRING){
					print_error("allowd_methods values needs to be a string");
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
	if (!(path.front() == '/' && path.back() == '/')){
		print_error("Location path needs to start and end with a /");
		return false;
	}
	loc.path = path;

	//root
	//if not set it will default to the server root value 
	it = locationblock.find("root");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			print_error("root value needs to be a string");
			return false;
		} else {
			loc.root = it->second->get<std::string>();
		}
	}

	//index
	it = locationblock.find("index");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::STRING){
			print_error("index value needs to be a string");
			return false;
		} else {
			//loc.index = it->second->get<std::string>();
			loc.set_index(it->second->get<std::string>());
		}
	}
	
	//auto_index
	it = locationblock.find("auto_index");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::BOOL){
			print_error("auto_index value needs to be a boolean");
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
			print_error("error_pages needs to be in the key pair format of two strings");
			return false;
		} else {
			njson::Json::object& error_pages = it->second->get<njson::Json::object>();
			njson::Json::object::iterator errit;
			for(errit = error_pages.begin(); errit != error_pages.end(); errit++){
				if (errit->second->get_type() != njson::Json::STRING){
					print_error("error_page value needs to be a string");
					return false;
				} else {
					if(!is_all_digits(errit->first)){
						print_error("error page key value needs to be all digits");
						return false;
					}
					try{
						loc.add_error_page(std::stoi(errit->first),errit->second->get<std::string>());
					} catch (std::exception e){
						print_error("couldn't process the error code");
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
			print_error("client_body_size value needs to be an integer");
			return false;
		} else {
			int body_size = it->second->get<int>();
			if(body_size < 0){
				print_error("client_body_size value can not be negative");
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
			print_error("redirect value need to be a string");
			return false;
		} else {
			loc.redirect = it->second->get<std::string>();
		}
	}

	//allowed http commands
	it = locationblock.find("allowed_methods");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			print_error("allowed_methods needs to be set in an array");
			return false;
		} else {
			njson::Json::array& allowed_http_commands = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < allowed_http_commands.size(); ++i){
				if(allowed_http_commands[i]->get_type() != njson::Json::STRING){
					print_error("allowd_methods values needs to be a string");
					return false;
				} else {
					loc.add_allowed_http_command(allowed_http_commands[i]->get<std::string>());
				}
			}
		}
	}
	
	//setting CGI
	it = locationblock.find("CGI");
	if(it != locationblock.end()){
		if(it->second->get_type() != njson::Json::ARRAY){
			print_error("CGI needs to be set in an array");
			return false;
		} else {
			njson::Json::array& cgi = it->second->get<njson::Json::array>();
			for(size_t i = 0; i < cgi.size(); ++i){
				if (cgi[i]->get_type() != njson::Json::STRING){
					print_error("CGI values needs to be a string");
					return false;
				} else {
					std::string ext = cgi[i]->get<std::string>();
					if (ext.front() != '.'){
						print_error("extensions for CGI have to start with a '.'");
						return false;
					}
					loc.add_cgi_extension(cgi[i]->get<std::string>());
				}
			}
		}
	}
	
	//setting upload_directory
	it = locationblock.find("upload_directory");
	if (it != locationblock.end()){
		if (it->second->get_type() != njson::Json::STRING){
			print_error("upload_directory needs to be a string");
			return false;
		} else {
			loc.upload_directory = it->second->get<std::string>();
		}
	}
	return true;
}

static bool process_locations(njson::Json::object& serverblock, Server* server){
	
	njson::Json::object::iterator it = serverblock.find("locations");
	if(it != serverblock.end()){
		if(it->second->get_type() != njson::Json::OBJECT){
			print_error("locations needs to be set in key value pairs");
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
		delete server;
		return nullptr;
	}

	njson::Json::object& serverblock = node->get<njson::Json::object>();
	// make sure that all directives in server block are supported
	if(!check_directives_server_block(serverblock)
	|| !set_server_variables(serverblock, server)
	|| !process_locations(serverblock, server)) {
		delete server;
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
	
	std::set<std::string> hosts_to_listen;

	for(size_t i = 0; i < servers.size(); ++i){
		std::string server_key = servers[i]->host + ':' + std::to_string(servers[i]->port);
		if(hosts_to_listen.count(server_key) == 0){
			hosts_to_listen.insert(server_key);
			Socket * sock_serv = new Socket(servers[i]->port, servers[i]->host);
			sock_serv->add_server_ref(servers[i]);
			sockets.emplace_back(sock_serv);
		} else {
			for(size_t j = 0; j < sockets.size(); ++j){
				std::string socket_key = sockets[j]->get_host() + ':' + std::to_string(sockets[j]->get_port());
				if(socket_key == server_key){
					sockets[j]->add_server_ref(servers[i]);
					break;
				}
			}
		}
	}
	// sockets.emplace_back(new ShellSocket(6666));
	return (sockets);
}

} // namespace webserv
