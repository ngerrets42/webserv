#ifndef SERVER_H
# define SERVER_H

#include "Core.h"
#include <map>
#include <utility>
#include "njson.h"

// the location block enables us to handle several types of URIs/routes within a server block
class Location{
	private:

		std::string						_root; //root directory on the server for the location
		std::vector<std::string>		_index; //index pages e.g. index.htm index.html
		bool							_autoindex; //show the directory listing if true else it won't
		std::map<int,std::string>		_error_pages; //custom error pages for the location. When not defined default will be used
		size_t							_max_size_client_body; // if request body is bigger, it will return error code 413 Request Entity Too Large. Current in bytes.
		std::vector<std::string>		_allowed_http_commands; //defines what HTTP request are allowed with this location
		std::pair<int, std::string>		_redirect;	//defines the redirect for this location. The redirect status code and the url
		std::unordered_map<std::string, std::string> _CGI; //the extension for the CGI and the path to the binary 
		std::string						_uploaded_file_storage_path; //the path where the uploaded file should be saved.

	public:
		std::string		get_root(void) const;
		std::vector<std::string>	get_index(void) const;
		bool	get_autoindex(void) const;
		std::map<int,std::string>	get_error_pages(void) const;
		size_t	get_max_size_client_body(void) const;
		std::vector<std::string>	get_allowed_http_commands(void) const;
		std::pair<int, std::string>	get_redirect(void) const;
		std::unordered_map<std::string, std::string>	get_CGI(void) const;
		std::string	get_uploaded_file_storage_path(void) const;
		bool	is_http_command_allowed(std::string const & http_command) const; //return if a http command is allowed on this location
		std::string	const &	request_error_page(int error_code); //returns the error page name/path if not found return a string object that is empty
};

class Server{
	//the server block contains the the server configuration directives
	public:
		Server(Json::pointer json);
		~Server();
	private:
		int							_port; //port of the virtual server if no port has been set the default port should be 80
		std::vector<std::string>	_server_names; //server names e.g. www.example.com example.com
		std::string					_root; //root directory of where the server directory starts
		std::string					_index; //index pages e.g. index.htm index.html
		std::map<int,std::string>	_error_pages; //default error page for the server. Will be used when no error page has been defined in the location block
		size_t						_max_size_client_body; // if request body is bigger, it will return error code 413 Request Entity Too Large. Current in bytes.
		std::pair<int, std::string>	_redirect;	//defines the redirect for this location. The redirect status code and the url

		std::vector<Location>		_locations; //stores all the locations blocks that has been defined for the server

	public:

		int		get_port(void) const;
		std::vector<std::string>	get_server_names(void) const;
		std::string					get_root(void) const;
		std::string					get_index(void) const;
		std::map<int, std::string>	get_error_pages(void) const;
		size_t						get_max_size_client_body(void) const;
		std::pair<int, std::string> get_redirect(void) const;
		std::vector<Location>		get_locations(void) const;

		// bool	contain_server_name(const std::string server_name) const;
};

std::vector<Server*> build_servers(Json* json); // This function loops through the Json creating a vector of servers and returning the vector.

#endif