#include "Connection.h"
#include "RequestHandler.h"

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t socket_fd, addr_t address)
:	socket_fd(socket_fd),
	address(address),
	last_request(),
	last_response(),
	busy(false) {}

Connection::~Connection() { close(socket_fd); }

Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Request& Connection::get_last_request(void) { return last_request; }
Response& Connection::get_last_response(void) { return last_response; }

} // webserv
