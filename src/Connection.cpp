#include "Connection.h"
#include "RequestHandler.h"
#include "data.h"

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t connection_fd, addr_t address)
:	socket_fd(connection_fd),
	address(address),
	last_request(),
	last_response(),
	state(IDLE),
	busy(false) {}

Connection::~Connection() { close(socket_fd); }

Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Request const& Connection::get_last_request(void) const { return last_request; }
Response const& Connection::get_last_response(void) const { return last_response; }

std::string Connection::get_ip(void) const
{
	char* cstr = inet_ntoa(reinterpret_cast<addr_in_t const*>(&address)->sin_addr);
	std::string as_str(cstr);
	return (as_str);
}

void Connection::on_pollin(void)
{
	// Receive request OR continue receiving in case of POST
	if (state != READING)
		new_request();
	else
		return ; // Continue reading POST
}

void Connection::on_pollout(void)
{
	// Send response OR continue sending response
	if (state != WRITING)
		new_response();
	else
		continue_response();
}

void Connection::new_request(void)
{
	state = READING;
	handler_data = HandlerData();
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
	});
	handler_data.current_request = request_build(handler_data.buffer);
	std::cout << ", " << get_request_string(handler_data.current_request.type) << " request for: " << handler_data.current_request.path;
}

void Connection::new_response(void)
{
	state = WRITING;

}

void Connection::continue_response(void)
{

	// After being done:
	state = IDLE;
}

} // webserv
