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

void request_print(Request const& request, std::ostream& out)
{
	out << "REQUEST:\n";
	if (request.validity == INVALID)
	{
		out << "INVALID" << std::endl;
		return ;
	}
	
	out << get_request_string(request.type);

	out << ' ' << request.path << ' ' << request.http_version << '\n';

	out << "Host: " << request.host << '\n';

	out << std::endl;
}

// WARNING: Uses some pointer-magic
static std::string* get_value_from_key(Request& request, std::string& key)
{
	static Request r_ref;
	static std::unordered_map<std::string, std::string*> accepted_keys {
		{ "Accept-Charset",			nullptr },
		{ "Accept-Encoding",		nullptr },
		{ "Accept-Language",		nullptr },
		{ "Authorization",			nullptr },
		{ "Expect",					nullptr },
		{ "From",					nullptr },
		{ "Host",					&r_ref.host },
		{ "If-Match",				nullptr },
		{ "If-Modified-Since",		nullptr },
		{ "If-None-Match",			nullptr },
		{ "If-Range",				nullptr },
		{ "If-Unmodified-Since",	nullptr },
		{ "Max-Forwards",			nullptr },
		{ "Proxy-Authorization",	nullptr },
		{ "Range",					nullptr },
		{ "Referer",				nullptr },
		{ "TE",						nullptr },
		{ "User-Agent",				nullptr }
	};

	if (accepted_keys.find(key) == accepted_keys.end())
		return (nullptr);
	if (accepted_keys.at(key) == nullptr)
		return (nullptr);
	uintptr_t ptr = reinterpret_cast<uintptr_t>(accepted_keys.at(key)) - reinterpret_cast<uintptr_t>(&r_ref);
	return (reinterpret_cast<std::string*>(ptr + reinterpret_cast<uintptr_t>(&request)));
}

// This consumes the part of the buffer that's used
Request request_build(std::vector<char>& buffer)
{
	Request request;
	if (buffer.empty())
		return (request);

	// Create a string-stream from the data
	buffer.push_back(0); // null-termination
	std::stringstream buffer_stream(buffer.data());
	std::string word;

	// First line of REQUEST
	buffer_stream >> word;
	request.type = get_request_type(word);
	if (request.type == UNKNOWN) return (request); // Unsupported request
	
	buffer_stream >> request.path;
	if (request.path.length() == 0)
		return (request); // No path
	
	buffer_stream >> request.http_version;
	if (request.http_version.length() == 0)
		return (request); // No HTTP version
	
	std::getline(buffer_stream, word); // skip line

	while (!buffer_stream.eof())
	{
		if (buffer_stream.peek() == '\r')
			break ;
		buffer_stream >> word;
		if (word.length() == 0)
			break ;
		// remove last character from word (the ':')
		word.erase(word.length() - 1);
		std::string* value = get_value_from_key(request, word);
		if (value == nullptr)
		{
			// Unknown key
			std::getline(buffer_stream, word);
			continue ;
		}
		buffer_stream.get(); // skip the space
		std::getline(buffer_stream, *value);
	}
	std::getline(buffer_stream, word);
	
	// consume the read part of the buffer
	buffer.erase(buffer.begin(), buffer.begin() + buffer_stream.tellg());
	// stored_buffer = buffer.substr(buffer_stream.tellg());
	
	// std::cout << "stored buffer: {" << stored_buffer << '}' << std::endl;

	request.validity = VALID;
	return (request);
}

} // namespace webserv
