#ifndef CONNECTION_H
# define CONNECTION_H

# include <sys/socket.h>
# include <string>

namespace webserv {

#define HTTP_HEADER_BUFFER_SIZE 8192

enum RequestValidity
{
	VALID,
	INVALID
};

enum RequestType
{
	GET,
	POST,
	DELETE
};

struct Request
{
	// is the request valid
	RequestValidity validity;

	// First line of request
	RequestType type;
	std::string path;
	std::string http_version;

	Request();
};

struct Response
{
	size_t size;
	char* data;
};

class Connection
{
	public:
	Connection(int socket_fd, struct sockaddr address);

	Request receive_request(void);
	void send_response(Response& response);

	private:
	// unused constructors
	Connection();
	Connection(Connection const& other);
	
	Connection& operator=(Connection const& other);

	// functions
	public: Request build_request(std::string buffer);

	private:
	int socket_fd;
	struct sockaddr address;
};


void request_print(Request const& request);

} // webserv

#endif // CONNECTION_H
