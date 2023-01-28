#include "Request.h"

namespace webserv {

Request::Request() : validity(INVALID) {}

RequestType get_request_type(std::string const& word)
{
	if (word == "GET")		return GET;
	if (word == "POST")		return POST;
	if (word == "DELETE")	return DELETE;
	return  UNKNOWN;
}

char const* get_request_string(RequestType type)
{
	switch (type)
	{
		case GET: return "GET";
		case POST: return "POST";
		case DELETE: return "DELETE";
		default: break;
	}
	return "UNKNOWN";
}

void request_print(Request const& request)
{
	std::cout << "REQUEST:\n";
	if (request.validity == INVALID)
	{
		std::cout << "INVALID" << std::endl;
		return ;
	}
	
	std::cout << get_request_string(request.type);

	std::cout << ' ' << request.path << ' ' << request.http_version << '\n';

	std::cout << "Host: " << request.host << '\n';

	std::cout << std::endl;
}

} // webserv
