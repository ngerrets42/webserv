#include "Connection.h"

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t socket_fd, addr_t address) : socket_fd(socket_fd), address(address) {}

Connection::~Connection() { close(socket_fd); }

Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Request Connection::receive_request(void)
{
	char buffer[HTTP_HEADER_BUFFER_SIZE];

	ssize_t recv_size = recv(socket_fd, buffer, HTTP_HEADER_BUFFER_SIZE, 0);
	if (recv_size < 0)
	{
		std::cerr << "recv() returned " << recv_size << " for Connection {" << socket_fd << "}: "
			<< strerror(errno)
			<< std::endl;
	}

	if (recv_size > 0)
	{
		buffer[recv_size] = '\0';
		return build_request(std::string{ buffer });
	}
	return (build_request(std::string{}));
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

Request Connection::build_request(std::string buffer)
{
	Request request;
	if (buffer.length() == 0)
		return (request);

	std::stringstream buffer_stream(buffer);
	std::string word;

	// First line of REQUEST
	buffer_stream >> word;
	if (word == "GET")
		request.type = GET;
	else if (word == "POST")
		request.type = POST;
	else if (word == "DELETE")
		request.type = DELETE;
	else
		return (request);
	
	buffer_stream >> request.path;
	if (request.path.length() == 0)
		return (request);
	
	buffer_stream >> request.http_version;

	while (!buffer_stream.eof())
	{
		buffer_stream >> word;
		if (word.length() == 0)
			continue ;
		// remove last character from word (the ':')
		word.erase(word.length() - 1);
		std::string* value = get_value_from_key(request, word);
		if (value == nullptr)
		{
			// Unknown key
			continue ;
		}
		buffer_stream.get(); // skip the space
		std::getline(buffer_stream, *value);
	}
	request.validity = VALID;
	return (request);
}

void Connection::send_response(Response& response)
{
	std::ifstream file;
	if (response.file_size > 0 && response.file_path.length() > 0)
	{
		file.open(response.file_path);
		if (!file)
		{
			// something went wrong, send error 500?
			return ;
		}
	}

	//	Send the header
	ssize_t send_size = send(socket_fd, response.header.data(), response.header.size(), 0);
	if (send_size < 0)
		std::cerr << "send(header) returned " << send_size << ", for Connection {" << socket_fd << '}' << std::endl;

	if (!file)
		return ;

	// Initialize a buffer to write to and read from
	static size_t const BUFFER_SIZE = 2048;
	std::vector<char> buffer;
	buffer.reserve(BUFFER_SIZE);

	// Send the entire file
	while (!file.eof())
	{
		file.read( reinterpret_cast<char*>(buffer.data()) , BUFFER_SIZE);

		send_size = send(socket_fd, buffer.data(), static_cast<size_t>(file.gcount()), 0);
		if (send_size < 0)
			std::cerr << "send(file) returned " << send_size << ", for Connection {" << socket_fd << '}' << std::endl;
	}
}

} // webserv
