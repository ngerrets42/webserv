#include "Connection.h"
#include "RequestHandler.h"
#include "Socket.h"
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

void Connection::on_pollin(Socket& socket)
{
	// Receive request OR continue receiving in case of POST
	switch (state)
	{
		case READY_TO_READ: new_request(); break;
		case READING: continue_request(); break;
		default: return;	
	}	
}

void Connection::on_pollout(Socket& socket)
{
	// Send response OR continue sending response
	switch (state)
	{
		case READY_TO_WRITE: new_response(socket); break;
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
	if (ext == ".html")
		return ("text/html");
	if (ext == ".ico")
		return "image/x-icon";
	return ("text/plain");
}

void Connection::new_response(Socket& socket)
{
	state = WRITING;
	handler_data.current_response = Response();

	// Get the Server from host
	Server& server = socket.get_server(handler_data.current_request.host);
	Location loc = server.get_location(handler_data.current_request.path);

	// std::cout << " LOCPATH: " << loc.path << "AUTOINDEX: " << loc.autoindex.first << '|' << loc.autoindex.second << ", INDEX: " << loc.index;

	std::string const& root = server.get_root(loc);

	std::string fpath = root + handler_data.current_request.path;
	
	handler_data.current_response.set_status_code("200");

	// path is a directory
	if (fpath.back() == '/')
	{
		std::string const& indexp = server.get_index_page(loc);
		std::cout << " indexp: " << indexp;
		if (!indexp.empty())
			fpath += indexp;
		else if (server.is_auto_index_on(loc))
		{
			std::cout << "\nBUILDING AUTO-INDEX\n";
			handler_data.custom_page = build_index(root + handler_data.current_request.path, handler_data.current_request.path);
			if (handler_data.custom_page.empty())
				handler_data.current_response.set_status_code("500");
			else
			{
				handler_data.current_response.content_length = std::to_string(handler_data.custom_page.size());
				handler_data.current_response.content_type = "text/html";
			}
		}
		else
			handler_data.current_response.set_status_code("404");
	}

	if (handler_data.custom_page.empty())
	{
		handler_data.current_response.content_length = std::to_string(
			data::get_file_size(fpath));

		handler_data.file.open(fpath);
		if (!handler_data.file)
		{
			handler_data.current_response.set_status_code("404");
			handler_data.current_response.content_length.clear();
		}
		size_t pos = fpath.find_last_of('.');
		if (pos != std::string::npos)
			handler_data.current_response.content_type = content_type_from_ext(fpath.substr(pos));
		else
			handler_data.current_response.content_type = "text/plain";
	}

	// In case of error-code
	if (handler_data.current_response.status_code != "200")
	{
		std::cout << ": ERROR " << handler_data.current_response.status_code;
		std::string error_path = server.get_error_page(std::stoi(handler_data.current_response.status_code), loc);
		handler_data.current_response.content_length.clear();
		if (!error_path.empty())
		{
			fpath = root + '/' + error_path;
			handler_data.current_response.content_length = std::to_string(
			data::get_file_size(fpath));

			handler_data.file.open(fpath);
			if (!handler_data.file)
				handler_data.current_response.content_length.clear();
			size_t pos = fpath.find_last_of('.');
			if (pos != std::string::npos)
				handler_data.current_response.content_type = content_type_from_ext(fpath.substr(pos));
			else
				handler_data.current_response.content_type = "text/plain";
		}
	}

	std::cout << ": " << fpath;

	if (handler_data.current_response.content_length.empty())
		handler_data.current_response.content_length = "0";
	handler_data.current_response.add_http_header("content-length", handler_data.current_response.content_length);

	if (handler_data.current_request.connection == "keep-alive")
		handler_data.current_response.add_http_header("connection", "keep-alive");
	else
		handler_data.current_response.add_http_header("connection", "close");

	if (!handler_data.current_response.content_type.empty())
		handler_data.current_response.add_http_header("content-type", handler_data.current_response.content_type);

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
