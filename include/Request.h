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

	//
	std::string host;

	Request();
};

void request_print(Request const& request);

} // webserv

#endif // REQUEST_H
