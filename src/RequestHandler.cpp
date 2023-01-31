#include "RequestHandler.h"

namespace webserv {

RequestHandler RequestHandler::s_request_handler = RequestHandler();

RequestHandler::RequestHandler() {}

void RequestHandler::async(Socket* socket, Connection* connection, sockfd_t fd)
{
	std::thread handler_thread(RequestHandler::async_thread, socket, connection, fd);
	handler_thread.detach();
}

// Return a buffer of data that should contain the header of the request
static std::vector<char> receive(sockfd_t fd, size_t max_size)
{
	std::vector<char> buffer(max_size);

	size_t attempts = 10;
	ssize_t recv_size = 0;
	while (attempts)
	{
		recv_size = recv(fd, buffer.data(), max_size, 0);
		if (recv_size > 0)
			break ;

		std::cerr << "recv() returned " << recv_size << " for {fd: " << fd
			<< "}, attempts remaining: " << attempts
			<< std::endl;
		--attempts;
		recv_size = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (static_cast<size_t>(recv_size) != max_size)
		buffer.resize(recv_size);
	return buffer;
}

static size_t get_file_size(std::string const& fpath)
{
	std::ifstream getl(fpath, std::ifstream::ate | std::ifstream::binary);
	if (!getl)
		return (0);
	size_t file_length = getl.tellg();
	getl.close();
	return (file_length);
}

static Response response_build(Request& request, Socket* socket)
{
	Response response;

	std::string fpath;
	fpath = "var/www/html" + request.path; // ROOT + REQUEST PATH

	if (fpath == "var/www/html/")
		fpath += "index.html";
	
	response.file_size = get_file_size(fpath);
	if (response.file_size == 0)
	{
		// Error code 404
		std::cerr << "FILE_SIZE == 0" << std::endl;
		return response;
	}

	std::ifstream file(fpath);
	if (!file)
		throw (std::runtime_error("Bad file: " + fpath));

	response.file_path = fpath;

	response.header = "HTTP/1.1 200 OK\r\n"
		+ std::string {"Content-Length: "} + std::to_string(response.file_size) + "\r\n"
		+ "Connection: Keep-Alive\r\n"
		+ "Content-Type: " + "text/html" + "\r\n"
		+ "\r\n";
	return (response);
}

// Sends the response header and possible a file if provided
static void send_response(Response& response, sockfd_t fd)
{
	std::ifstream file;
	if (response.file_size > 0 && response.file_path.length() > 0)
	{
		file.open(response.file_path);
		if (!file)
		{
			// something went wrong, send error 500?
			std::cerr << "can't open file." << std::endl;
		}
	}

	//	Send the header
	ssize_t send_size = send(fd, response.header.data(), response.header.size(), 0);
	if (send_size < 0)
		std::cerr << "send(header) returned " << send_size << ", for Connection {" << fd << '}' << std::endl;

	if (!file)
		return ;

	// Initialize a buffer to write to and read from
	std::vector<char> buffer(MAX_SEND_BUFFER_SIZE);

	// Send the entire file
	while (!file.eof())
	{
		file.read( reinterpret_cast<char*>(buffer.data()) , buffer.size());

		send_size = send(fd, buffer.data(), static_cast<size_t>(file.gcount()), 0);
		if (send_size < 0)
			std::cerr << "send(file) returned " << send_size << ", for Connection {" << fd << '}' << std::endl;
	}
}

void RequestHandler::async_thread(Socket* socket, Connection* connection, sockfd_t fd)
{
	std::vector<char> buffer = receive(fd, HTTP_HEADER_BUFFER_SIZE);

	if (buffer.size() <= 0)
		return ; // Nothing to do
	
	Request request = request_build(buffer);
	// request_print(request);

	Response response = response_build(request, socket);
	send_response(response, fd);

	return ;
}

} // webserv
