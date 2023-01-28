#ifndef CONNECTION_H
# define CONNECTION_H

# include "Core.h"

# include "Request.h"
# include "Response.h"

namespace webserv {

#define HTTP_HEADER_BUFFER_SIZE 8192
#define HTTP_RESPONSE_MIN_THREAD_SIZE 8192

class Connection
{
	public:
	Connection(sockfd_t socket_fd, addr_t address);
	~Connection();

	Request receive_request(void);
	void send_response(Response& response);

	private:
	// unused constructors
	Connection();
	Connection(Connection const& other);
	
	Connection& operator=(Connection const& other);

	// functions
	private:
	Request build_request(std::string buffer);
	void build_request_get(Request& request, std::stringstream& buffer);
	void build_request_post(Request& request, std::stringstream& buffer);
	void build_request_delete(Request& request, std::stringstream& buffer);

	private:
	sockfd_t socket_fd;
	addr_t address;

	std::string stored_buffer;
};

} // webserv

#endif // CONNECTION_H
