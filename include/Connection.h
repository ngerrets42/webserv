#ifndef CONNECTION_H
# define CONNECTION_H

# include "Core.h"

# include "Request.h"
# include "Response.h"

namespace webserv {

#define HTTP_HEADER_BUFFER_SIZE 8192

class Connection
{
	public:
	enum State
	{
		READY_TO_READ = 0,
		READING,
		READY_TO_WRITE,
		WRITING,
		CLOSE
	};

	public:
	Connection(sockfd_t socket_fd, addr_t address);
	~Connection();

	Request const& get_last_request(void) const;
	Response const& get_last_response(void) const;

	std::string get_ip(void) const;

	State get_state(void) const;

	void on_pollin(void);
	void on_pollout(void);

	private:
	// unused constructors
	Connection();
	Connection(Connection const& other);
	
	Connection& operator=(Connection const& other);

	// functions
	private:

	void new_request(void);
	void continue_request(void);

	void new_response(void);
	void continue_response(void);

	Request build_request(std::string buffer);
	void build_request_get(Request& request, std::stringstream& buffer);
	void build_request_post(Request& request, std::stringstream& buffer);
	void build_request_delete(Request& request, std::stringstream& buffer);

	private:
	sockfd_t socket_fd;
	addr_t address;

	Request last_request;
	Response last_response;

	State state;

	struct HandlerData
	{
		Request current_request;
		Response current_response;
		std::vector<char> buffer;
		std::ifstream file;
	} handler_data;

	public:
	bool busy;
};

} // webserv

#endif // CONNECTION_H
