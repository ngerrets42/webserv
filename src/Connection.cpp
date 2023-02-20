#include "Connection.h"
#include "RequestHandler.h"
#include "data.h"
#include "html.h"

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t connection_fd, addr_t address)
:	socket_fd(connection_fd),
	address(address),
	last_request(),
	last_response(),
	state(READY_TO_READ),
	busy(false) {}

Connection::~Connection() { close(socket_fd); }
Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Request const& Connection::get_last_request(void) const { return last_request; }
Response const& Connection::get_last_response(void) const { return last_response; }

Connection::State Connection::get_state(void) const { return state; }

std::string Connection::get_ip(void) const
{
	char* cstr = inet_ntoa(reinterpret_cast<addr_in_t const*>(&address)->sin_addr);
	std::string as_str(cstr);
	return (as_str);
}

void Connection::on_pollin(void)
{
	// Receive request OR continue receiving in case of POST
	switch (state)
	{
		case READY_TO_READ: new_request(); break;
		case READING: continue_request(); break;
		default: return;	
	}	
}

void Connection::on_pollout(void)
{
	// Send response OR continue sending response
	switch (state)
	{
		case READY_TO_WRITE: new_response(); break;
		case WRITING: continue_response(); break;
		default: return;
	}
}

void Connection::new_request(void)
{
	state = READING;
	handler_data = HandlerData();
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
		std::cout << "ON_ZERO!" << std::endl;
	});
	handler_data.current_request = request_build(handler_data.buffer);

	// Set last_request for debugging purposes
	last_request = handler_data.current_request;

	std::cout << ", " << get_request_string(handler_data.current_request.type) << " request for: " << handler_data.current_request.path;

	continue_request();
}

void Connection::continue_request(void)
{
	// after done
	state = READY_TO_WRITE;
}

static std::string content_type_from_ext(std::string const& ext)
{
	return ("text/plain");
}

void Connection::new_response(void)
{
	state = WRITING;
	handler_data.current_response = Response();

	handler_data.current_response.set_status_code("200");
	if (handler_data.current_request.connection == "keep-alive")
		handler_data.current_response.add_http_header("connection", "keep-alive");
	else
		handler_data.current_response.add_http_header("connection", "close");
	if (handler_data.current_request.path.back() == '/')
	{
		handler_data.custom_page = build_index("var/www/html" + handler_data.current_request.path, handler_data.current_request.path);
		handler_data.current_response.content_length = std::to_string(handler_data.custom_page.size());
		handler_data.current_response.content_type = "text/html";
	}
	else
	{
		handler_data.current_response.content_length = std::to_string(
			data::get_file_size("var/www/html" + handler_data.current_request.path));
		
		handler_data.current_response.content_type = "text/html";

		handler_data.file.open("var/www/html" + handler_data.current_request.path);
		if (!handler_data.file)
		{
			handler_data.current_response.set_status_code("404");
			handler_data.current_response.content_length = "0";
		}
	}

	data::send(socket_fd, handler_data.current_response.get_response());

	// Set last_response for debugging purposes
	last_response = handler_data.current_response;

	continue_response();
}

void Connection::continue_response(void)
{
	if (!handler_data.custom_page.empty())
	{
		data::send(socket_fd, handler_data.custom_page);
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.connection == "keep-alive")
			state = READY_TO_READ;
		else
			std::cout << "connection: " << handler_data.current_request.connection << std::endl;
	}
	else if (!data::send_file(socket_fd, handler_data.file, MAX_SEND_BUFFER_SIZE))
	{
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.connection == "keep-alive")
			state = READY_TO_READ;
		else
			std::cout << "connection: |" << handler_data.current_request.connection << "|" << std::endl;
	}
}

} // namespace webserv
