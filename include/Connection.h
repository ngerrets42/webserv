#ifndef CONNECTION_H
# define CONNECTION_H

# include "Core.h"
# include "Pollable.h"
# include "Request.h"
# include "Response.h"
# include "Server.h"
# include "CGI.h"

namespace webserv {

#define HTTP_HEADER_BUFFER_SIZE 8192
#define CONNECTION_LIFETIME 10

class Socket;

class Connection : public Pollable
{
	public:
	enum State
	{
		READY_TO_READ = 0,	// Waiting for REQUEST
		READING,			// Receiving the body of the REQUEST
		READY_TO_WRITE,		// Ready to send RESPONSE
		WRITING,			// Sending body of RESPONSE
		CLOSE				// Connection needs to be closed
	};

	Connection(sockfd_t connection_fd, addr_t address, Socket* parent);
	virtual ~Connection();

	private:
	// unused constructors
	Connection();
	Connection(Connection const& other);
	
	Connection& operator=(Connection const& other);

	public:
	// Getters
	Request const& get_last_request(void) const;
	Response const& get_last_response(void) const;
	std::string get_ip(void) const;
	State get_state(void) const;
	virtual sockfd_t get_fd(void) const override;

	virtual bool should_destroy(void) const override;

	virtual short get_events(sockfd_t fd) const override;

	void reset_time_remaining(void);
	virtual void on_post_poll(pollable_map_t& fd_map) override;

	protected:
	virtual void on_pollin(pollable_map_t& fd_map) override;
	virtual void on_pollout(pollable_map_t& fd_map) override;
	virtual void on_pollhup(pollable_map_t& fd_map, sockfd_t fd) override;
	// virtual void on_pollnval(pollable_map_t& fd_map) override;

	// functions
	private:

	void new_request(pollable_map_t& fd_map);
	void new_request_cgi(pollable_map_t& fd_map);
	void continue_request(void);

	void new_response(void);
	void new_response_get(Server const& server, Location const& loc);
	void new_response_cgi(Server const& server, Location const& loc);
	void new_response_delete(Server const& server, Location const& loc);
	void new_response_redirect(Server const& server, Location const& loc);
	void continue_response(pollable_map_t& fd_map);

	Request build_request(std::string buffer);
	void build_request_get(Request& request, std::stringstream& buffer);
	void build_request_post(Request& request, std::stringstream& buffer);
	void build_request_delete(Request& request, std::stringstream& buffer);

	private:
	sockfd_t socket_fd;
	addr_t address;
	Socket* parent;

	Request last_request;
	Response last_response;

	State state;

	size_t last_time;

	struct HandlerData
	{
		Request current_request;
		Response current_response;
		std::string custom_page;
		std::vector<char> buffer;
		std::ifstream file;
		size_t content_size;
		size_t received_size;
		CGI* cgi;
		HandlerData();
	} handler_data;

};

} // namespace webserv

#endif // CONNECTION_H
