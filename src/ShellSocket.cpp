#include "ShellSocket.h"
#include "Command.h"
#include "RequestHandler.h"

namespace webserv {

ShellSocket::ShellSocket(uint16_t port) : Socket(port), password("1234") {}
ShellSocket::~ShellSocket() { close(socket_fd); }

// Unavailable constructors
// ShellSocket::ShellSocket() : socket_fd(-1) {};
// ShellSocket::ShellSocket(ShellSocket const& other) { (void)other; }
// ShellSocket& ShellSocket::operator=(ShellSocket const& other) { (void)other; return *this; }

void ShellSocket::on_request(sockfd_t fd, Connection* connection)
{
	std::cout << "ShellSocket-event: POLLIN";
	std::cout << std::endl;
	
	static size_t const SIZE = 256;
	std::vector<char> buffer = receive(fd, SIZE);

	if (buffer.back() == '\n')
		buffer.pop_back();

	buffer.push_back('\0');

	std::string as_str = std::string(buffer.data());

	std::cout << " > Received CMD: " << as_str << std::endl;

	std::stringstream response_buffer;
	if (verified.count(fd) == 0)
	{
		// attempt verification
		if (as_str == password)
		{
			verified.insert(fd);
			response_buffer << "Password correct, verified client. Enter commands:\n";
		}
		else
			response_buffer << "Password incorrect, re-enter password:\n";
		send(fd, response_buffer.str().c_str(), response_buffer.str().size() + 1, 0);
		return ;
	}
	Command* cmd = Command::find(as_str);
	if (cmd == nullptr)
		response_buffer << "Command not found\n";
	else
		cmd->run(response_buffer);
	send(fd, response_buffer.str().c_str(), response_buffer.str().size() + 1, 0);
}

} // webserv
