#ifndef REQUEST_H
# define REQUEST_H

# include "Core.h"

namespace webserv {

enum RequestValidity
{
	VALID,
	INVALID
};

enum RequestType
{
	UNKNOWN,
	GET,
	POST,
	DELETE
};

struct Request
{
	// is the request valid
	RequestValidity validity;

	// First line of request
	RequestType type;
	std::string path;
	std::string http_version;
	std::unordered_map<std::string, std::string> path_arguments;
	std::unordered_map<std::string, std::string> fields;

	Request();
};

void request_print(Request const& request, std::ostream& out = std::cout);
RequestType get_request_type(std::string const& word);
char const* get_request_string(RequestType type);
Request request_build(std::vector<char>& buffer);

} // webserv

#endif // REQUEST_H
