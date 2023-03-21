#ifndef SERVER_H
# define SERVER_H

#include "Core.h"
#include <algorithm>
#include <utility>

namespace webserv{

// the location block enables us to handle several types of URIs/routes within a server block
class Location{
	public:
		std::string										path; //the URI of the location block
		std::string										root; //(inherit if not defined) root directory for the location.
		std::string										index; //(inherit if not defined)index page name if not defined will inherit from server
		std::pair<bool, bool>							autoindex;//(inherit if not defined) //show the directory listing if true else it won't
		std::unordered_map<int,std::string>				error_pages; //(inherit if not defined)custom error pages for the location. When not defined will inherit from server
		std::pair<bool, size_t>							client_max_body_size; // (inherit if not defined)if request body is bigger, it will return error code 413 Request Entity Too Large. Current in bytes. 0 means disabled
		std::vector<std::string>						allowed_http_commands; //(inherit if not defined)defines what HTTP request are allowed with this location. 
		std::string										redirect;	//defines the redirect for this location. The redirect will be code 301 for permanent redirect and this will contain the url that is being redirected to
		std::vector<std::string>						cgi; //contains the cgi extentions the location support
		std::string										upload_directory; //defines the path for storing files

		Location(void);
		Location(std::string const & path); //constructor to create a Location object with the path set
		~Location(void);

		//setters
		void								add_error_page(int error_code, std::string const & error_page);
		void								add_allowed_http_command(std::string const & http_command);
		void								set_auto_index(bool autoindex_setting);
		void								set_client_max_body_size(size_t body_size);
		void								add_cgi_extension(std::string const & extention); //add the cgi extention and the path to the script
		void								set_upload_directory(std::string const & upload_dir);

		//getters
		std::string							get_error_page(int error_code) const; //returns the error page name/path if no error pages been defined return a string object that is empty 
		bool								is_http_command_allowed(std::string const & http_command) const; //return if a http command is allowed on this location
		std::string							get_cgi_path(std::string const & path); //returns the path that is related with the extention else return empty string
};

class Server{
	//the server block contains the the server configuration directives

	private:
		size_t	match_paths(std::string const & input_path, std::string const & loc_block_path);
		bool	find_http_command(std::vector<std::string> const & http_commands, std::string const & http_command) const;

	public:
		int										port; //port of the virtual server if no port has been set the default port should be 80
		std::string								host; //the host to which the bind will be applied to. for anything it will be "0.0.0.0" or the ipaddress of the interface of the computer 
		std::vector<std::string>				server_names; //server names e.g. www.example.com example.com  could also be ip adresses, all optional

		std::string								root; //root directory of where the server directory starts. Will be inherited by location is not defined in location default value of no root is defined in config file
		std::string								index; //index page name. will be inherited by location if not defined there. Default value is index.html even if it has not been initialized
		bool									autoindex; //show the directory listing if true else it won't
		std::unordered_map<int, std::string>	error_pages; //default error page for the server. Will be used when no error page has been defined in the location block
		size_t									client_max_body_size; // if request body is bigger, it will return error code 413 Request Entity Too Large. Current in bytes. 0 means disabled. will be inherited by locations unless other wise defined
		std::vector<std::string>				allowed_http_commands; //defines what HTTP request are allowed with this location. 
		std::string								redirect;	//defines the redirect for this location. The redirect will be code 301 for permanent redirect and this will contain the and this will contain the url that is being redirected to
		
		std::vector<Location>					locations; //stores all the locations blocks that has been defined for the server. The string is the path and the Location object is the location block

		Server(void);
		~Server(void);
		
		//setters
		void								add_server_name(std::string const & server_name); //add server names to the server block
		void								add_error_page(int error_code, std::string const & error_page); //add error page to the server block
		void								add_allowed_http_command(std::string const & http_command); //add allowed HTTP commands for the server block
		void								add_location(Location const & location_block); //add location to the server block

		//getters
		Location 							get_location(std::string const & loc_path); //will return the best matching location block. if not will return nullpointer
		bool								contain_server_name(std::string const & server_name) const; //return true if the server name has been found in the list
		bool								is_http_command_allowed(std::string const & http_command, Location const & location) const; //return if a http command is allowed on this location
		std::string							get_error_page(int error_code, Location const & location) const; //returns the error page name/path if no error pages been defined return a string object that is empty 
		std::string const & 				get_root(Location const & location) const; //get the root of a particular location
		size_t								get_client_max_body_size(Location const & location) const; //will return the client max body size for that location
		bool								is_auto_index_on(Location const & location) const; //will return true autoindex for location is on
		std::string const &					get_index_page(Location const & location) const; //will return the index page for the location
		std::pair<std::string, std::string>	get_cgi(Location & location, std::string const & path) const; //will return the path to the cgi binary or script
		std::string const &					get_redirection(Location const & location) const; //will return the url of the redirection if set
		std::string const &					get_upload_dir(Location const & location) const; // will return the upload path set in the location block
};

} //namespace webserv

#endif
