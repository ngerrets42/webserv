#include "Server.h"

namespace webserv{

//==============================================================================
//Location class
//==============================================================================

Location::Location(void):autoindex(std::make_pair(false, false)), client_max_body_size(std::make_pair(false, 0)){}

Location::Location(std::string const & loc_path):path(loc_path){}

Location::~Location(void){}

//setters

void Location::add_error_page(int error_code, std::string const & error_page){
	error_pages[error_code] = error_page;
}

void Location::add_allowed_http_command(std::string const & http_command){
	allowed_http_commands.push_back(http_command);
}

void Location::set_auto_index(bool autoindex_setting){
	autoindex.first = true;
	autoindex.second = autoindex_setting;
}

void Location::set_client_max_body_size(size_t body_size){
	client_max_body_size.first = true;
	client_max_body_size.second = body_size;
}

void Location::add_cgi_extension(std::string const & extention){
	cgi.push_back(extention);
}

void Location::set_upload_path(std::string const & upload_path){
	upload_path_loc = upload_path;
}


//getters

//the return is not const & because if the error page is not found it will return a empty string back that is local to this function
std::string Location::get_error_page(int error_code) const{
	std::unordered_map<int, std::string>::const_iterator it = error_pages.find(error_code);
	if(it == error_pages.end()){
		return (std::string());
	} else {
		return it->second;
	}
}

bool Location::is_http_command_allowed(std::string const & http_command) const{
	if(allowed_http_commands.size() == 0)
		return true;
	std::vector<std::string>::const_iterator it = std::find(allowed_http_commands.begin(), allowed_http_commands.end(), http_command);
	if (it == allowed_http_commands.end()){
		return false;
	} else {
		return true;
	}
}

std::string Location::get_cgi_path(std::string const & path) {
	if(cgi.empty() || (path.find('.',0) == std::string::npos)){
		return std::string();
	}
	for (size_t i = 0; i < cgi.size(); ++i){
		size_t ext_size = cgi[i].size();
		size_t pos = path.find(cgi[i], 0);
		while (pos != std::string::npos){ 
			if (path[pos + ext_size] == '/' || path[pos + ext_size] == '\0'){
				return path.substr(0, pos + ext_size);
			}
			pos = path.find(cgi[i], pos + 1);
		}
	}
	return std::string();
}

//==============================================================================
//Server class
//==============================================================================
//Server default constructor.
//	it will set some values to default
//		port		80
//		root		/var/www/html
//		autoindex	false
//		client_max_body_size	0 (meaning no limit)

Server::Server(void):port(80),root("/var/www/html"), index(""),autoindex(false), client_max_body_size(0){}

Server::~Server(void){};


//this function will compare the input_path with loc_block_path and return how much input_path match the loc_block_path.
//return the amount of characters is matches from left to right. It will return 0 if it doesn't match.
size_t Server::match_paths(std::string const & input_path, std::string const & loc_block_path){
	
	if(input_path.empty() || loc_block_path.empty() || input_path.size() < loc_block_path.size()){
		return 0;
	}
	size_t size_loc_block_path = loc_block_path.size();
	if(input_path.compare(0, size_loc_block_path, loc_block_path) == 0){
		return size_loc_block_path;
	}
	return 0;
}

//setters

void	Server::add_server_name(std::string const & server_name){
	server_names.push_back(server_name);
}

void Server::add_error_page(int error_code, std::string const & error_page){
	error_pages[error_code] = error_page;
}

void Server::add_allowed_http_command(std::string const & http_command){
	allowed_http_commands.push_back(http_command);
}

void Server::add_location(Location const & location_block){
	locations.push_back(location_block);
}

//getters

Location	Server::get_location(std::string const & loc_path){
	
	Location	current_match;
	size_t		current_match_length = 0;
	size_t		result = 0;

	for(size_t i = 0; i < locations.size(); ++i){
		result = match_paths(loc_path,locations[i].path);
		if(result > current_match_length){
			current_match = locations[i];
			current_match_length = result;
		}
	}
	return current_match;
}

bool	Server::contain_server_name(std::string const & server_name) const{
	std::vector<std::string>::const_iterator it = std::find(server_names.begin(), server_names.end(),server_name);
	if(it == server_names.end()){
		return false;
	} else {
		return true;
	}
}

bool	Server::find_http_command(std::vector<std::string> const & http_commands, std::string const & http_command) const{
	std::vector<std::string>::const_iterator it = std::find(http_commands.begin(), http_commands.end(), http_command);
	if (it == http_commands.end()){
		return false;
	} else {
		return true;
	}
}

bool Server::is_http_command_allowed(std::string const & http_command, Location const & location) const{
	if(location.allowed_http_commands.empty()){
		if(allowed_http_commands.empty()){
			return true;
		}
		return find_http_command(allowed_http_commands, http_command);
	} else {
		return find_http_command(location.allowed_http_commands, http_command);
	}
}

std::string	Server::get_error_page(int error_code, Location const & location) const{
	std::unordered_map<int, std::string>::const_iterator it;
	if(location.error_pages.empty()){
		if(error_pages.empty()){
			return std::string();
		}
	} else {
		it = location.error_pages.find(error_code);
		if(it != error_pages.end()){
			return it->second;
		}
	}
	it = error_pages.find(error_code);
	if(it == error_pages.end()){
		return std::string();
	} else {
		return it->second;
	}
}

std::string const & Server::get_root(Location const & location) const{
	if(location.root.empty()){
		return root;
	} else {
		return location.root;
	}
}

size_t Server::get_client_max_body_size(Location const & location) const{
	if (location.client_max_body_size.first == true){
		return location.client_max_body_size.second;
	} else {
		return client_max_body_size;
	}
}

bool	Server::is_auto_index_on(Location const & location) const{
	if (location.autoindex.first == true){
		return location.autoindex.second;
	} else {
		return autoindex;
	}
}

std::string const &	Server::get_index_page(Location const & location) const{
	if (location.index.empty()){
		return index;
	} else {
		return location.index;
	}
}

std::pair<std::string, std::string> Server::get_cgi(Location & location, std::string const & path) const{
	std::pair<std::string, std::string> result;
	if(location.cgi.empty()){
		return result;
	} else {
		std::string cgi_path = location.get_cgi_path(path);
		if (cgi_path.empty()){
			return result;
		} else {
			result.first = cgi_path;
			if (path.size() > cgi_path.size()){
				result.second = path.substr(cgi_path.size(), path.size() - cgi_path.size());
			}
			return result;
		}
	}
}

std::string const & Server::get_redirection(Location const & location) const{
	if (location.redirect.empty()){
		return redirect;
	} else {
		return location.redirect;
	}
}

} //namespace webserv
