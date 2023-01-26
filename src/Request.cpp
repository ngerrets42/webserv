#include "Request.h"

namespace webserv {

Request::Request() : validity(INVALID) {}

void request_print(Request const& request)
{
	std::cout << "REQUEST:\n";
	if (request.validity == INVALID)
	{
		std::cout << "INVALID" << std::endl;
		return ;
	}
	
	if (request.type == GET) std::cout << "GET";
	if (request.type == POST) std::cout << "POST";
	if (request.type == DELETE) std::cout << "DELETE";

	std::cout << ' ' << request.path << ' ' << request.http_version << '\n';

	std::cout << "Host: " << request.host << '\n';

	std::cout << std::endl;
}

} // webserv
