#include "Connection.h"

#include <sys/socket.h>
#include <iostream>
#include <sstream>

namespace webserv {

// CONSTRUCTORS
Connection::Connection(int socket_fd, struct sockaddr address) : socket_fd(socket_fd), address(address) {}

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
		std::cerr << "recv() returned " << recv_size << ". Can not read from Connection {" << socket_fd << "}"<< std::endl;
	}

	if (recv_size > 0)
		return build_request(std::string{ buffer });
	return (build_request(std::string{}));
}

Request Connection::build_request(std::string buffer)
{
	Request request;
	if (buffer.length() == 0)
		return (request);

	std::stringstream buffer_stream(buffer);
	std::string word;
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
	}
	request.validity = VALID;
	return (request);
}

void Connection::send_response(Response& response)
{
	ssize_t send_size = send(socket_fd, response.data, response.size, 0);
	if (send_size < 0)
	{
		std::cerr << "send() returned " << send_size << ", for Connection {" << socket_fd << '}' << std::endl;
	}
}

// REQUEST
Request::Request() : validity(INVALID) {}

void request_print(Request const& request)
{
	std::cout << "REQUEST: ";
	if (request.validity == INVALID)
	{
		std::cout << "INVALID" << std::endl;
		return ;
	}
	
	if (request.type == GET) std::cout << "GET";
	if (request.type == POST) std::cout << "POST";
	if (request.type == DELETE) std::cout << "DELETE";

	std::cout << ' ' << request.path << ' ' << request.http_version;

	std::cout << std::endl;
}

} // webserv
