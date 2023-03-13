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
	std::string path_arguments;
	std::unordered_map<std::string, std::string> fields;

	Request();
};

void request_print(Request const& request, std::ostream& out = std::cout);
RequestType get_request_type(std::string const& word);
char const* get_request_string(RequestType type);
void parse_header_fields(std::unordered_map<std::string, std::string>& fields, std::vector<char>& buffer, std::stringstream& buffer_stream);
Request request_build(std::vector<char>& buffer);

} // namespace webserv

#endif // REQUEST_H
