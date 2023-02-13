#include "RequestHandler.h"

#include "data.h"

namespace webserv {

RequestHandler RequestHandler::s_request_handler = RequestHandler();

RequestHandler::RequestHandler() {}

// static std::string const& get_errorpage_desc(std::string code)
// {
// 	static std::unordered_map<std::string, std::string> const desc_map = {
// 		{"_", "UNKNOWN"},
// 		{"404", "Not Found"}
// 	};

// 	if (desc_map.find(code) == desc_map.end())
// 		return (desc_map.at("_"));
// 	return (desc_map.at(code));
// }

// static Response errorpage(std::string code, Server& server, Location& location)
// {
// 	Response response = {};

// 	if (code.length() != 3)
// 		throw (std::runtime_error("ERRORCODE needs to have 3 characters"));

// 	// response.file_path = server.get_errorpage_path(code, location);
// 	// response.file_size = get_file_size(response.file_path);

// 	response.header = "HTTP/1.1 " + code + ' ' + get_errorpage_desc(code) + "\r\n";
// 	response.header += "Content-Length: " + std::to_string(response.file_size) + "\r\n";
// 	response.header += "\r\n";
// 	return (response);
// }

// static Response response_build(Request& request, Socket* socket)
// {
// 	Response response;

// 	if (request.validity == INVALID)
// 	{
// 		Server server = Server();
// 		Location location = Location();
// 		return (errorpage("400", server, location));
// 	}

// 	std::string fpath;

// 	if (request.path == "/")
// 		request.path = "/index.html"; // HARDCODED FOR NOW

// 	// Get Server based on host:
// 	Server server = Server(); // TODO: Actually get server from host

// 	// Get Location based on request path:
// 	Location location = Location(); // TODO: Actually get location from path

// 	fpath = "var/www/html" + request.path; // TODO: ROOT + REQUEST PATH
	
// 	response.file_size = get_file_size(fpath);
// 	if (response.file_size == 0)
// 		return errorpage("404", server, location);

// 	std::string ext = request.path.substr(request.path.find_last_of('.') + 1);
// 	std::string content_type;
// 	if (ext == "html")
// 		content_type = "text/html";
// 	else if (ext == "png")
// 		content_type = "image/png";
// 	else if (ext == "ico")
// 		content_type = "image/x-icon";
// 	else
// 		content_type = "text/plain";

// 	std::ifstream file(fpath);
// 	if (!file)
// 		return errorpage("404", server, location);

// 	response.file_path = fpath;

// 	response.header = "HTTP/1.1 200 OK\r\n"
// 		+ std::string {"Content-Length: "} + std::to_string(response.file_size) + "\r\n"
// 		+ "Connection: Keep-Alive\r\n"
// 		+ "Content-Type: " + content_type + "\r\n"
// 		+ "\r\n";
// 	return (response);
// }

// // Sends the response header and possible a file if provided
// static void send_response(Response& response, sockfd_t fd)
// {
// 	std::ifstream file;
// 	if (response.file_size > 0 && response.file_path.length() > 0)
// 	{
// 		file.open(response.file_path);
// 		if (!file)
// 		{
// 			// something went wrong, send error 500?
// 			std::cerr << "can't open file." << std::endl;
// 		}
// 	}

// 	//	Send the header
// 	ssize_t send_size = send(fd, response.header.data(), response.header.size(), 0);
// 	if (send_size < 0)
// 		std::cerr << "send(header) returned " << send_size << ", for Connection {" << fd << '}' << std::endl;

// 	if (!file)
// 		return ;

// 	// Initialize a buffer to write to and read from
// 	std::vector<char> buffer(MAX_SEND_BUFFER_SIZE);

// 	// Send the entire file
// 	while (!file.eof())
// 	{
// 		file.read( reinterpret_cast<char*>(buffer.data()) , buffer.size());
// 		if (file.gcount() <= 0)
// 			std::cerr << "file.read() gcount = " << file.gcount() << std::endl;

// 		send_size = -1;
// 		static const size_t MAX_ATTEMPTS = 100;
// 		size_t attempts = 0;
// 		while (send_size < 0)
// 		{
// 			send_size = send(fd, buffer.data(), static_cast<size_t>(file.gcount()), 0);
// 			if (send_size < 0)
// 			{
// 				++attempts;
// 				if (attempts >= MAX_ATTEMPTS)
// 				{
// 					std::cerr << "MAX_ATTEMPTS reached for Connection {" << fd << '}' << std::endl;
// 					// close?
// 					return ;
// 				}
// 				// std::cerr << "send(file) returned " << send_size << ", for Connection {" << fd << '}' << std::endl;
// 				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for a little bit then try again
// 			}
// 			else
// 				attempts = 0; // reset attempts
// 		}
// 	}
// }

// void receive_post(sockfd_t fd, Socket* socket, Request& request, std::vector<char>& buffer)
// {
// 	std::string file_path = request.path;
// 	size_t recv_data = buffer.size();

// 	std::cout << "receive_post() fpath: " << file_path << std::endl;
// 	std::string root = "var/www/upload";
// 	std::ofstream file(root + file_path);
// 	if (!file)
// 		throw(std::runtime_error("Can't open file for writing"));
	
// 	while (buffer.size() > 0)
// 	{
// 		file.write(buffer.data(), buffer.size());
// 		static size_t MAX_POST_RECV_SIZE = 2042;
// 		buffer = data::receive(fd, MAX_POST_RECV_SIZE);
// 	}
// }

// void RequestHandler::handle(Socket* socket, Connection* connection, sockfd_t fd)
// {
// 	if (connection->busy)
// 		return ;
// 	connection->busy = true;

// 	std::vector<char> buffer = data::receive(fd, HTTP_HEADER_BUFFER_SIZE);

// 	if (socket->get_connection(fd) == nullptr)
// 		return ;
// 	if (buffer.size() <= 0)
// 	{
// 		connection->busy = false;
// 		return ; // Nothing to do
// 	}

// 	Request request;
// 	Response response;
// 	// Request& request = connection->get_last_request();
// 	// request = request_build(buffer);
// 	// // request_print(request);

// 	// Response& response = connection->get_last_response();
// 	// response = response_build(request, socket);

// 	if (request.type == POST && request.validity == VALID)
// 		receive_post(fd, socket, request, buffer);

// 	send_response(response, fd);
// 	connection->busy = false; // release connection
// 	return ;
// }

} // webserv
