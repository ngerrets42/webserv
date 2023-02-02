#ifndef SOCKET_H
# define SOCKET_H

# include "Core.h"

# include "Server.h"
# include "Connection.h"

namespace webserv {

class Socket
{
	public:
	// Constructors
	Socket(uint16_t port);

	~Socket();

	private:
	// Private Constructors
	Socket();
	Socket(Socket const& other);
	Socket& operator=(Socket const& other);

	public:
	void notify(sockfd_t fd, short revents, std::unordered_map<sockfd_t, Socket*>& fd_map);

	/**
	 * @brief check if fd is busy.
	 * 
	 * @param fd one of the descriptors this Socket is responsible for.
	 * @return true if ready for another task or the fiven fd is the Socket's own fd.
	 * @return false if busy.
	 */
	bool is_active(sockfd_t fd) const;

	std::vector<Server*> get_servers(void);

	sockfd_t get_socket_fd(void);

	protected:
	virtual void on_request(sockfd_t fd, Connection* connection);
	void accept_connections(std::unordered_map<sockfd_t, Socket*>& fd_map);

	protected:
	sockfd_t socket_fd;
	addr_in_t address;

	std::vector<Server*> servers;
	std::unordered_map<sockfd_t, Connection*> connection_map; // map of Connection socket fd - Connection class
};

} // webserv

#endif // SOCKET_H
