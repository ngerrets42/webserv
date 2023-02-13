#ifndef SHELL_SOCKET_H
# define SHELL_SOCKET_H

# include "Core.h"
# include "Socket.h"

namespace webserv {

class ShellSocket : public Socket
{
	public:
	// Constructors
	ShellSocket(uint16_t port);

	~ShellSocket();

	private:
	// Private Constructors
	// ShellSocket();
	// ShellSocket(ShellSocket const& other);
	// ShellSocket& operator=(ShellSocket const& other);

	protected:
	virtual void on_pollin(sockfd_t fd, Connection* connection);

	private:
	std::set<sockfd_t> verified;
	std::string password;
	
};

} // webserv

#endif // SHELL_SOCKET_H
