#include "Connection.h"
#include "Core.h"
#include "Request.h"
#include "Socket.h"
#include "CGI.h"
#include "data.h"
#include "html.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <sstream>
#include <sys/_types/_ssize_t.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

namespace webserv {

// TODO: Delete connection when CLOSED

// CONSTRUCTORS
Connection::Connection(sockfd_t connection_fd, addr_t address, Socket* parent)
:	socket_fd(connection_fd),
	address(address),
	parent(parent),
	state(READY_TO_READ) {}

// Unused
Connection::~Connection() { close(socket_fd); }
Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Connection::HandlerData::HandlerData()
:	content_size(0),
	received_size(0),
	cgi(nullptr) {}

// POLL
void Connection::on_pollhup(pollable_map_t& fd_map)
{
	// Connection interrupted, remove self from fd_map
	std::cout << "Connection::on_pollhup" << std::endl;
	fd_map.erase(get_fd());
}

void Connection::on_pollin(pollable_map_t& fd_map)
{
	std::cout << "Connection::on_pollin: ";
	// Receive request OR continue receiving in case of POST
	switch (state)
	{
		case READY_TO_READ: new_request(fd_map); break;
		case READING: continue_request(); break;
		default: return;
	}
}

void Connection::on_pollout(pollable_map_t& fd_map)
{
	std::cout << "Connection::on_pollout\n";
	// Send response OR continue sending response
	switch (state)
	{
		case READY_TO_WRITE: new_response(); break;
		case WRITING: continue_response(fd_map); break;
		default: return;
	}

	if (state == CLOSE)
	{
		fd_map.erase(get_fd());
		delete this;
	}
}

short Connection::get_events(sockfd_t fd) const
{
	(void)fd;
	short events = POLLHUP | POLLIN;
	if (state == READY_TO_WRITE || state == WRITING)
		events |= POLLOUT;
	return (events);
}

void Connection::new_request_post(pollable_map_t& fd_map)
{
	// Build cgi-environment
	std::vector<std::string> env = cgi::env_init();
	// cgi::env_set_value(env, "REMOTE_USER", "hman");
	cgi::env_set_value(env, "CONTENT_LENGTH",handler_data.current_request.fields["content-length"]);
	cgi::env_set_value(env, "CONTENT_TYPE", handler_data.current_request.fields["content-type"]);
	// cgi::env_set_value(env, "SCRIPT_FILENAME", "cgi-bin/handle_form.php");
	cgi::env_set_value(env, "REQUEST_METHOD", "POST");

	Server& serv = parent->get_server(handler_data.current_request.fields["host"]);
	Location loc = serv.get_location(handler_data.current_request.path);
	handler_data.cgi = new CGI(env, serv, loc);

	handler_data.cgi->buffer_in = handler_data.buffer; // Push leftover buffer into the CGI buffer
	handler_data.cgi->buffer_in.pop_back();

	fd_map.insert({handler_data.cgi->get_in_fd(), handler_data.cgi});
	fd_map.insert({handler_data.cgi->get_out_fd(), handler_data.cgi});

	try { handler_data.content_size = std::stoul(handler_data.current_request.fields["content-length"]); }
	catch (std::exception& e)
	{
		// TODO: Error-code & Close CGI
		std::cerr << e.what() << std::endl;
	}
	handler_data.received_size = handler_data.buffer.size();
	std::cout << "received data " << handler_data.received_size << '/' << handler_data.content_size;

	if (handler_data.received_size >= handler_data.content_size)
		// Already received everything
		state = READY_TO_WRITE;
}

// Request building
void Connection::new_request(pollable_map_t& fd_map)
{
	state = READING;
	handler_data = HandlerData();
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
	});
	handler_data.current_request = request_build(handler_data.buffer);

	// Build the CGI
	if (handler_data.current_request.type == POST)
		new_request_post(fd_map);
	else
		state = READY_TO_WRITE;

	// Set last_request for debugging purposes
	last_request = handler_data.current_request;

	std::cout << ", " << get_request_string(handler_data.current_request.type) << " request for: " << handler_data.current_request.path;
}

void Connection::continue_request(void)
{
	// Only happens during POST usually
	// Receive data from connection
	if (!handler_data.cgi->buffer_in.empty())
	{
		// std::cout << "Buffer not empty!" << std::endl;
		return ;
	}

	handler_data.cgi->buffer_in = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&](){
		std::cout << "SETTING STATE TO CLOSE" << std::endl;
		this->state = CLOSE;
	});

	handler_data.received_size += handler_data.cgi->buffer_in.size();
	std::cout << "receiving data " << handler_data.received_size << '/' << handler_data.content_size << std::endl;

	// Write to CGI
	// std::cout << "Writing buffer to CGI: {" << (char*)handler_data.buffer.data() << "}" << std::endl;
	// ssize_t write_size = write(handler_data.pipes.in[1], handler_data.buffer.data(), handler_data.buffer.size());
	// if (write_size != static_cast<ssize_t>(handler_data.buffer.size()))
	// {
	// 	// TODO: Error-code. Something went wrong, can't write full size
	// 	std::cerr << "Can't write full buffer to CGI";
	// }

	if (state == CLOSE)
		return ;

	if (handler_data.received_size >= handler_data.content_size)
	{
		state = READY_TO_WRITE;
	}
}

static std::string content_type_from_ext(std::string const& path)
{
	static std::string const default_type("text/plain");
	static std::unordered_map<std::string, std::string> const ext_type_map {
		{"html", "text/html"},
		{"ico", "image/x-icon"}
	};

	size_t pos = path.find_last_of('.');
	if (pos != std::string::npos)
	{
		++pos;
		if (pos >= path.length()) return (default_type);
		auto it = ext_type_map.find(path.substr(pos));
		if (it != ext_type_map.end()) return (it->second);
	}
	return (default_type);
}

// Response building
void Connection::new_response(void)
{
	state = WRITING;
	handler_data.current_response = Response();

	// Get the Server from host
	Socket& socket = *parent;
	Server& server = socket.get_server(handler_data.current_request.fields["host"]);
	Location loc = server.get_location(handler_data.current_request.path);
	
	handler_data.current_response.set_status_code("200");

	switch (handler_data.current_request.type)
	{
		case GET: new_response_get(server, loc); break;
		case POST: new_response_post(server, loc); break;
		case DELETE: new_response_delete(server, loc); break;
		default: break; // TODO: error-page
	}

	if (state == READY_TO_WRITE)
		return ;

	// In case of error-code
	if (handler_data.current_response.status_code != "200")
	{
		std::cout << ": ERROR " << handler_data.current_response.status_code;
		std::string error_path = server.get_error_page(std::stoi(handler_data.current_response.status_code), loc);
		handler_data.current_response.content_length.clear();
		if (!error_path.empty())
		{
			std::string fpath = server.get_root(loc) + '/' + error_path;
			handler_data.current_response.content_length = std::to_string(
			data::get_file_size(fpath));

			handler_data.file.open(fpath);
			if (!handler_data.file)
				handler_data.current_response.content_length.clear();
			handler_data.current_response.content_type = content_type_from_ext(fpath);
		}
	}

	// Add final headers
	// 	handler_data.current_response.content_length = "0";

	if (!handler_data.current_response.content_length.empty())
		handler_data.current_response.add_http_header("content-length", handler_data.current_response.content_length);

	if (handler_data.current_request.fields["connection"] == "keep-alive")
		handler_data.current_response.add_http_header("connection", "keep-alive");
	else
		handler_data.current_response.add_http_header("connection", "close");

	if (!handler_data.current_response.content_type.empty())
		handler_data.current_response.add_http_header("content-type", handler_data.current_response.content_type);

	// Send the response header
	data::send(socket_fd, handler_data.current_response.get_response());

	// Set last_response for debugging purposes
	last_response = handler_data.current_response;

	// continue_response();
}

void Connection::new_response_get(Server const& server, Location const& loc)
{
	std::string const& root = server.get_root(loc);
	std::string fpath = root + handler_data.current_request.path;

	// path is a directory
	if (fpath.back() == '/')
	{
		std::string const& indexp = server.get_index_page(loc);
		if (!indexp.empty())
			fpath += indexp;
		else if (server.is_auto_index_on(loc))
		{
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

	// No custom page (index), so we have to get the file from fpath (or send 404 not found)
	if (handler_data.custom_page.empty())
	{
		handler_data.current_response.content_length = std::to_string(
			data::get_file_size(fpath));

		// Open file, no file = 404
		handler_data.file.open(fpath);
		if (!handler_data.file)
		{
			handler_data.current_response.set_status_code("404");
			handler_data.current_response.content_length.clear();
		}
		// determine content type based on extention
		handler_data.current_response.content_type = content_type_from_ext(fpath);
	}

	std::cout << ": " << fpath;
}

void Connection::new_response_post(Server const& server, Location const& loc)
{
	// Read buffer from CGI-output
	// std::vector<char> buffer(MAX_SEND_BUFFER_SIZE);
	// ssize_t read_size = read(handler_data.pipes.out[0], buffer.data(), MAX_SEND_BUFFER_SIZE);
	// if (read_size < 0)
	// {
	// 	std::cout << " cgi didn't output any data";
	// 	state = READY_TO_WRITE;
	// 	return ;
	// }
	// if (static_cast<size_t>(read_size) != MAX_SEND_BUFFER_SIZE)
	// 	buffer.resize(read_size);

	// handler_data.buffer = buffer;

	// buffer.push_back('\0');

	if (handler_data.cgi->buffer_out.empty())
	{
		state = READY_TO_WRITE;
		return ;
	}

	handler_data.current_response.content_type = "text/plain";

	// parse into request
	std::unordered_map<std::string, std::string> fields;

	handler_data.cgi->buffer_out.push_back('\0');
	std::stringstream buffer_stream(handler_data.cgi->buffer_out.data());
	parse_header_fields(fields, handler_data.cgi->buffer_out, buffer_stream);
	// handler_data.cgi->buffer_out.erase(handler_data.cgi->buffer_out.end() - 1);
	
	// TODO: read "Status" header-field

	auto it = fields.find("content-type");
	if (it != fields.end()) handler_data.current_response.content_type = it->second;

	// TODO: handle no content-length
	it = fields.find("content-length");
	if (it != fields.end()) handler_data.current_response.content_length = it->second;
	else
	{
		handler_data.current_request.fields["connection"] = "close";
		handler_data.current_response.content_length.clear();
	}
}

void Connection::new_response_delete(Server const& server, Location const& loc)
{

}

void Connection::continue_response(pollable_map_t& fd_map)
{
	if (handler_data.current_request.type == POST)
	{
		if (handler_data.cgi->buffer_out.empty())
			return ;

		ssize_t send_data = data::send(socket_fd, handler_data.cgi->buffer_out);
		if (send_data != handler_data.cgi->buffer_out.size())
		{
			// TODO: Error-code. Something went wrong, can't write full size
			std::cerr << "Can't write full buffer to CGI";
		}

		handler_data.cgi->buffer_out.clear();

		int wstatus;
		int rpid = waitpid(handler_data.cgi->get_pid(), &wstatus, WNOHANG);
		if (rpid <= 0)
		{
			fd_map.erase(handler_data.cgi->get_in_fd());
			fd_map.erase(handler_data.cgi->get_out_fd());
			delete handler_data.cgi;
			handler_data.cgi = nullptr;
			std::cout << ", CGI finished execution, exitcode: " << WEXITSTATUS(wstatus) << std::endl;
			state = CLOSE; // Close is default unless keep-alive
			if (handler_data.current_request.fields["connection"] == "keep-alive")
				state = READY_TO_READ;
		}
	}
	else if (!handler_data.custom_page.empty())
	{
		ssize_t send_data = data::send(socket_fd, handler_data.custom_page);
		if (static_cast<size_t>(send_data) != handler_data.custom_page.size())
			std::cerr << "send_data of custom_page == " << send_data << std::endl;
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
		handler_data.custom_page.clear();
	}
	else if (!data::send_file(socket_fd, handler_data.file, MAX_SEND_BUFFER_SIZE))
	{
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
	}
}

// GETTERS
Request const& Connection::get_last_request(void) const { return last_request; }
Response const& Connection::get_last_response(void) const { return last_response; }
Connection::State Connection::get_state(void) const { return state; }

sockfd_t Connection::get_fd(void) const
{
	return (socket_fd);
}

std::string Connection::get_ip(void) const
{
	char* cstr = inet_ntoa(reinterpret_cast<addr_in_t const*>(&address)->sin_addr);
	std::string as_str(cstr);
	return (as_str);
}

} // namespace webserv
