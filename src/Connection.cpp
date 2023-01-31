#include "Connection.h"
#include "RequestHandler.h"

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t socket_fd, addr_t address) : socket_fd(socket_fd), address(address) {}

Connection::~Connection() { close(socket_fd); }

Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

// Request Connection::receive_request(void)
// {


// 	char buffer[HTTP_HEADER_BUFFER_SIZE];

// 	ssize_t recv_size = recv(socket_fd, buffer, HTTP_HEADER_BUFFER_SIZE, 0);
// 	if (recv_size < 0)
// 	{
// 		std::cerr << "recv() returned " << recv_size << " for Connection {" << socket_fd << "}: "
// 			<< std::strerror(errno)
// 			<< std::endl;
// 	}

// 	if (recv_size > 0)
// 		return build_request(std::string(buffer, recv_size));
// 	return (build_request(std::string{}));
// }


// void Connection::build_request_get(Request& request, std::stringstream& buffer)
// {

// }

// void Connection::send_response(Response& response)
// {
// 	std::cout << "file path: " << response.file_path << std::endl;

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
// 	ssize_t send_size = send(socket_fd, response.header.data(), response.header.size(), 0);
// 	if (send_size < 0)
// 		std::cerr << "send(header) returned " << send_size << ", for Connection {" << socket_fd << '}' << std::endl;

// 	if (!file)
// 		return ;

// 	// Initialize a buffer to write to and read from
// 	static size_t const BUFFER_SIZE = 2048;
// 	std::vector<char> buffer;
// 	buffer.reserve(BUFFER_SIZE);

// 	// Send the entire file
// 	while (!file.eof())
// 	{
// 		file.read( reinterpret_cast<char*>(buffer.data()) , BUFFER_SIZE);

// 		send_size = send(socket_fd, buffer.data(), static_cast<size_t>(file.gcount()), 0);
// 		if (send_size < 0)
// 			std::cerr << "send(file) returned " << send_size << ", for Connection {" << socket_fd << '}' << std::endl;
// 	}
// }

} // webserv
