#ifndef REQUEST_HANDLER_H
# define REQUEST_HANDLER_H

# include "Core.h"

# include "Socket.h"
# include "Request.h"

namespace webserv {

# define HTTP_HEADER_BUFFER_SIZE 8192
# define MAX_SEND_BUFFER_SIZE 10

class RequestHandler
{
	public:
	static void async(Socket* socket, Connection* connection, sockfd_t fd);

	private:
	RequestHandler();

	static void async_thread(Socket* socket, Connection* connection, sockfd_t fd);

	private:
	static RequestHandler s_request_handler;
};

std::vector<char> receive(sockfd_t fd, size_t max_size);

} // webserv

#endif // REQUEST_HANDLER_H
