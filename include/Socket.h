#ifndef SOCKET_H
# define SOCKET_H

# include "Core.h"

# include "Pollable.h"
# include "Server.h"

namespace webserv {

# define MAX_SOCKET_QUEUE 100

class Socket : public Pollable
{
	public:
	// Constructors
	Socket(uint16_t _port, std::string const& host = "0.0.0.0");

	virtual ~Socket();

	private:
	// Private Constructors
	Socket();
	Socket(Socket const& other);
	Socket& operator=(Socket const& other);

	public:
	// GETTERS
	uint16_t get_port(void) const;
	std::string const& get_host(void) const;
	Server& get_server(std::string const& host);
	std::vector<Server*> get_servers(void);
	void add_server_ref(std::unique_ptr<Server>& server_ref);

	virtual sockfd_t get_fd(void) const override;

	virtual bool should_destroy(void) const override;

	virtual short get_events(sockfd_t fd) const override;

	protected:
	virtual void on_pollin(pollable_map_t& fd_map) override;
	virtual void on_pollout(pollable_map_t& fd_map) override;
	// virtual void on_pollhup(pollable_map_t& fd_map, sockfd_t fd) override;

	void accept_connections(pollable_map_t& fd_map);

	protected:
	uint16_t port;
	std::string host;
	sockfd_t socket_fd;
	addr_in_t address;

	std::vector<Server*> servers;
};

} // namespace webserv

#endif // SOCKET_H
