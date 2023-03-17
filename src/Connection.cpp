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
#include <ctime>
#include <exception>
#include <sstream>
#include <sys/_types/_size_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

namespace webserv {

// CONSTRUCTORS
Connection::Connection(sockfd_t connection_fd, addr_t address, Socket* parent)
:	socket_fd(connection_fd),
	address(address),
	parent(parent),
	state(READY_TO_READ) { reset_time_remaining(); }

// Unused
Connection::~Connection()
{
	std::cout << '(' << socket_fd << "): " << "Connection closed and destroyed." << std::endl;
	close(socket_fd);
}

Connection::Connection() : socket_fd(-1) {}
Connection::Connection(Connection const& other) { (void)other; }
Connection& Connection::operator=(Connection const& other) { (void)other; return *this; }
//END

Connection::HandlerData::HandlerData()
:	content_size(0),
	received_size(0),
	cgi(nullptr) {}

void Connection::reset_time_remaining(void)
{
	// std::cout << "time reset" << std::endl;
	last_time = std::time(nullptr);
}

// POLL
void Connection::on_pollnval(pollable_map_t& fd_map)
{
	std::cout << "Connection::on_pollnval" << std::endl;
}

void Connection::on_post_poll(pollable_map_t& fd_map)
{
	if (handler_data.cgi != nullptr)
	{
		int wstatus;
		int rpid = waitpid(handler_data.cgi->get_pid(), &wstatus, WNOHANG);
		if (rpid > 0)
		{
			fd_map.erase(handler_data.cgi->get_in_fd());
			fd_map.erase(handler_data.cgi->get_out_fd());
			delete handler_data.cgi;
			handler_data.cgi = nullptr;
			std::cout << "CGI finished execution, exitcode: " << WEXITSTATUS(wstatus) << std::endl;
		}
	}

	size_t curr_time = std::time(nullptr);
	// std::cout << CONNECTION_LIFETIME - (curr_time - last_time) << std::endl;
	if (curr_time - last_time >= CONNECTION_LIFETIME)
		state = CLOSE;
}

void Connection::on_pollhup(pollable_map_t& fd_map, sockfd_t fd)
{
	(void)fd_map;
	(void)fd;
	// Connection interrupted, remove self from fd_map
	// std::cout << "Connection::on_pollhup" << std::endl;

	if (handler_data.cgi != nullptr)
	{
		if (handler_data.cgi->get_in_fd() != -1)
			std::cout << '(' << socket_fd << "): " << "Waiting for CGI to finish..." << std::endl;
		handler_data.cgi->close_pipes();
		int wstatus;
		int rpid = waitpid(handler_data.cgi->get_pid(), &wstatus, WNOHANG);
		if (rpid > 0)
		{
			fd_map.erase(handler_data.cgi->get_in_fd());
			fd_map.erase(handler_data.cgi->get_out_fd());
			delete handler_data.cgi;
			handler_data.cgi = nullptr;
			std::cout << '(' << socket_fd << "): " << "CGI finished execution, exitcode: " << WEXITSTATUS(wstatus) << std::endl;
		}
	}
	// Set self to close, so the connection can be closed by an external observer
	state = CLOSE;
}

void Connection::on_pollin(pollable_map_t& fd_map)
{
	std::cout <<'(' << socket_fd << "): " <<  "Connection::on_pollin"  << std::endl;
	// Receive request OR continue receiving in case of POST
	switch (state)
	{
		case READY_TO_READ: new_request(fd_map); break;
		case READING: continue_request(); break; // TODO: If reading too long send 408 REQUEST TIMEOUT
		default: return;
	}
}

void Connection::on_pollout(pollable_map_t& fd_map)
{
	// std::cout << "Connection::on_pollout"  << std::endl;
	// Send response OR continue sending response
	switch (state)
	{
		case READY_TO_WRITE: new_response(); break;
		case WRITING: continue_response(fd_map); break;
		default: return;
	}

	// if (state == CLOSE)
	// {
	// 	fd_map.erase(get_fd());
	// 	delete this;
	// }
}

short Connection::get_events(sockfd_t fd) const
{
	(void)fd;
	short events = POLLHUP;
	// if (handler_data.cgi != nullptr)
	// {
	// 	if (!handler_data.cgi->buffer_out.empty())
	// 		events |= POLLOUT;
	// } else
	if (state == READY_TO_WRITE || state == WRITING)
		events |= POLLOUT;
	else if (state == READING || state == READY_TO_READ)
	{
		// if (handler_data.cgi != nullptr)
		// {
		// 	if (handler_data.cgi->buffer_in.empty())
		// 		events |= POLLIN;
		// }
		// else
			events |= POLLIN;
	}
	return (events);
}

void Connection::new_request_cgi(pollable_map_t& fd_map)
{
	// cgi::env_set_value(env, "SCRIPT_FILENAME", "cgi-bin/handle_form.php");
	// Build cgi-environment

	Server& serv = parent->get_server(handler_data.current_request.fields["host"]);
	Location loc = serv.get_location(handler_data.current_request.path);

	auto cgi_pair = serv.get_cgi(loc, handler_data.current_request.path);

	std::vector<std::string> env = env::initialize();
	auto it = handler_data.current_request.fields.find("content-length");
	if (it != handler_data.current_request.fields.end())
		env::set_value(env, "CONTENT_LENGTH", it->second);
	
	if (handler_data.current_request.type == POST)
		env::set_value(env, "REQUEST_METHOD", "POST");
	else
		env::set_value(env, "REQUEST_METHOD", "GET");
	
	if (handler_data.current_request.fields.find("content-type") != handler_data.current_request.fields.end())
		env::set_value(env, "CONTENT_TYPE", handler_data.current_request.fields["content-type"]);
	

	env::set_value(env, "REMOTE_ADDR", get_ip());
	env::set_value(env, "REMOTE_HOST", get_ip());
	env::set_value(env, "GATEWAY_INTERFACE", "CGI/1.1");
	// env::set_value(env, "REMOTE_USER", "TeamWebserv"); // UNUSED
	// env::set_value(env, "REMOTE_IDENT", "TeamWebserv"); // UNUSED

	env::set_value(env, "SERVER_NAME", "webserv");
	env::set_value(env, "SERVER_PROTOCOL", "HTTP/1.1");
	env::set_value(env, "SERVER_SOFTWARE", "webserv");
	env::set_value(env, "SCRIPT_NAME", cgi_pair.first);
	env::set_value(env, "PATH_INFO", cgi_pair.second);
	env::set_value(env, "PATH_TRANSLATED",  serv.get_root(loc) + cgi_pair.second);
	env::set_value(env, "QUERY_STRING", handler_data.current_request.path_arguments);

	// No content length means no body to send to the CGI
	if (handler_data.current_request.fields.find("content-length") == handler_data.current_request.fields.end())
		state = READY_TO_WRITE;
	else
	{
		// Read content length
		try { handler_data.content_size = std::stoul(handler_data.current_request.fields["content-length"]); }
		catch (std::exception& e)
		{
			std::cerr << '(' << socket_fd << "): " << "Connection::new_request_cgi(): " << e.what() << std::endl;
			handler_data.current_response.set_status_code("500");
			return ;
		}
	}

	try { handler_data.cgi = new CGI(env, serv, loc, handler_data.current_request.path); }
	catch (std::exception& e)
	{
		std::cerr << '(' << socket_fd << "): " << "Connection::new_request_cgi(): " << e.what() << std::endl;
		// delete handler_data.cgi;
		handler_data.current_response.set_status_code("500");
		return ;
	}

	handler_data.cgi->buffer_in = handler_data.buffer; // Push leftover buffer into the CGI buffer
	handler_data.cgi->buffer_in.pop_back();

	// Add fds to map for polling
	fd_map.insert({handler_data.cgi->get_in_fd(), handler_data.cgi});
	fd_map.insert({handler_data.cgi->get_out_fd(), handler_data.cgi});

	// Amount of data already received
	handler_data.received_size = handler_data.cgi->buffer_in.size();
	std::cout << '(' << socket_fd << "): " << "received data " << handler_data.received_size << '/' << handler_data.content_size << " - ";

	// We received everything already, no need to continue READING
	if (handler_data.received_size >= handler_data.content_size)
		// Already received everything
		state = READY_TO_WRITE;
}

// Request building
void Connection::new_request(pollable_map_t& fd_map)
{
	reset_time_remaining();
	// Initial request and response conditions
	state = READING;
	handler_data = HandlerData();
	handler_data.current_response = Response();

	// receive the HTTP header
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
	});
	handler_data.current_request = request_build(handler_data.buffer);

	Server& server = parent->get_server(handler_data.current_request.fields["host"]);
	Location loc = server.get_location(handler_data.current_request.path);

	// Check for index page and alter path
	if (handler_data.current_request.path.back() == '/')
	{
		std::string const& indexp = server.get_index_page(loc);
		if (!indexp.empty())
			handler_data.current_request.path += indexp;
	}

	// Get the content length for validation check
	size_t content_length = 0;
	if (handler_data.current_request.fields.find("content-length") != handler_data.current_request.fields.end())
	{
		try { content_length = std::stoul(handler_data.current_request.fields["content-length"]); }
		catch (std::exception& e) { handler_data.current_response.set_status_code("403"); }
	}

	// Initial request validations
	if (handler_data.current_request.validity == INVALID) handler_data.current_response.set_status_code("400");
	else if (!server.is_http_command_allowed(get_request_string(handler_data.current_request.type), loc))
		handler_data.current_response.set_status_code("405");
	else if ( std::set<std::string>{"HTTP/0.9", "HTTP/1.0", "HTTP/1.1"}.count(handler_data.current_request.http_version) == 0)
		handler_data.current_response.set_status_code("505");
	else if (server.get_client_max_body_size(loc) != 0 && content_length > server.get_client_max_body_size(loc))
		handler_data.current_response.set_status_code("413");

	// When there is no status response code
	if (handler_data.current_response.status_code.empty())
	{
		// Build the CGI
		auto cgi_pair = server.get_cgi(loc, handler_data.current_request.path);
		if (!cgi_pair.first.empty())
		{
			std::string cgi = server.get_root(loc) + "/" + cgi_pair.first;
			if (data::get_file_size(cgi) == 0)
			{
				handler_data.current_response.set_status_code("404");
				state = READY_TO_WRITE;
			}
			else
				new_request_cgi(fd_map);
		}
		else
		{
			state = READY_TO_WRITE;
			if (handler_data.current_request.type == DELETE)
			{
				// Remove file
				std::string to_remove = server.get_root(loc) + handler_data.current_request.path;
				std::cout << "removing file: " << to_remove << std::endl;
				if( ::remove(to_remove.c_str()) != 0 )
				{
					std::cerr << "Can't delete file: " << strerror(errno) << std::endl;
					handler_data.current_response.set_status_code("404");
				}
			}
		}
	}
	else state = READY_TO_WRITE;

	// Set last_request for debugging purposes
	last_request = handler_data.current_request;

	std::cout << '(' << socket_fd << "): "
		<< get_request_string(handler_data.current_request.type)
		<< " request for: " << handler_data.current_request.path
		<< std::endl;
}

void Connection::continue_request(void)
{
	if (handler_data.cgi == nullptr)
	{
		std::cout << '(' << socket_fd << "): " << "CGI no longer exists." << std::endl;
		state = CLOSE;
		return ;
	}

	// Receive data from connection
	static size_t cgi_counter = 0;
	int wstatus;
	int rpid = waitpid(handler_data.cgi->get_pid(), &wstatus, WNOHANG);
	if (rpid > 0)
	{
		std::cout << '(' << socket_fd << "): " << "CGI finished execution, exitcode: " << WEXITSTATUS(wstatus) << std::endl;
		state = READY_TO_WRITE;
		return ;
	}

	// if (!handler_data.cgi->buffer_in.empty())
	// {
	// 	std::cout << "buffer_in not empty" << std::endl;
	// 	cgi_counter++;
	// 	if (cgi_counter > 10000)
	// 	{
	// 		handler_data.cgi->close_pipes();
	// 		std::cout << "CGI pipes closed" << std::endl;
	// 	}
	// 	return ;
	// }

	cgi_counter = 0;


	handler_data.cgi->buffer_in = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&](){
		std::cerr << "SETTING STATE TO CLOSE" << std::endl;
		this->state = CLOSE;
	});

	handler_data.received_size += handler_data.cgi->buffer_in.size();
	std::cout << '(' << socket_fd << "): "
		<< "receiving data " << handler_data.received_size << '/' << handler_data.content_size << std::endl;

	if (state == CLOSE)
		return ;

	if (handler_data.received_size >= handler_data.content_size)
		state = READY_TO_WRITE;
	
	reset_time_remaining();
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

	if (handler_data.cgi != nullptr)
	{
		if (!handler_data.cgi->buffer_in.empty())
		{
			// Give CGI more time
			return ;
		}
		else if (handler_data.cgi->get_in_fd() != -1)
		{
			std::cout << '(' << socket_fd << "): " << "No more input, closing CGI-in" << std::endl;
			// handler_data.cgi->close_in();
			close(handler_data.cgi->get_in_fd());
		}
	}

	// Get the Server from host
	Socket& socket = *parent;
	Server& server = socket.get_server(handler_data.current_request.fields["host"]);
	Location loc = server.get_location(handler_data.current_request.path);
	
	if (handler_data.current_response.status_code.empty() || handler_data.current_response.status_code == "200" || handler_data.current_response.status_code == "201")
	{
		if (!server.get_redirection(loc).empty())
			new_response_redirect(server, loc);
		else if (handler_data.current_request.type == DELETE)
			new_response_delete(server, loc);
		else
		{
			if (handler_data.cgi != nullptr)
				new_response_cgi(server, loc);
			else
				new_response_get(server, loc);
		}
	}

	// if (handler_data.cgi != nullptr && handler_data.cgi->destroy)
	// {
	// 	std::cout << "deleting cgi" << std::endl;
	// 	delete handler_data.cgi;
	// 	state = CLOSE;
	// 	// return ;
	// }

	if (handler_data.cgi != nullptr && handler_data.cgi->buffer_out.empty())
	{
		if (handler_data.cgi->get_out_fd() == -1)
		{
			handler_data.current_response.set_status_code("500");
		}
		else
		{
			state = READY_TO_WRITE;
			return ;
		}
	}

	reset_time_remaining();

	// In case of error-code
	if (!handler_data.current_response.status_code.empty()
		&& handler_data.current_response.status_code.front() != '2' // Codes starting with 2 are OK etc
		&& handler_data.current_response.status_code.front() != '3') // Codes starting with 3 are redirects
	{
		std::cout << '(' << socket_fd << "): "
			<< "ERROR " << handler_data.current_response.status_code << std::endl;

		handler_data.current_request.fields["connection"] = "close";
		std::string error_path = server.get_error_page(std::stoi(handler_data.current_response.status_code), loc);

		handler_data.current_response.content_length = "0";
		if (!error_path.empty())
		{
			std::string fpath = server.get_root(loc) + '/' + error_path;
			handler_data.current_response.content_length = std::to_string(
			data::get_file_size(fpath));

			handler_data.file.open(fpath, std::ios_base::binary);
			if (!handler_data.file)
			{
				std::cerr << " open failed; " << std::endl;
				handler_data.current_response.content_length = "0";
			}
			handler_data.current_response.content_type = content_type_from_ext(fpath);
		}
	}

	if (handler_data.current_response.status_code.empty())
	{
		if (handler_data.current_request.type == POST)
			handler_data.current_response.set_status_code("201");
		else
		 	handler_data.current_response.set_status_code("200");
	}

	// Add final headers

	if (!handler_data.current_response.content_length.empty())
		handler_data.current_response.add_http_header("content-length", handler_data.current_response.content_length);

	// if (handler_data.current_response.content_length.empty() && handler_data.current_request.fields["connection"] == "keep-alive")
	// 	handler_data.current_request.fields["connection"] = "close";

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
		if (server.is_auto_index_on(loc))
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
		{
			handler_data.current_response.set_status_code("404");
			return ;
		}
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
}

void Connection::new_response_cgi(Server const& server, Location const& loc)
{
	if (handler_data.cgi->buffer_out.empty())
		return ;
	std::cout << "Connection::new_response_cgi" << std::endl;

	handler_data.current_response.content_type = "text/plain";

	// parse into request
	std::unordered_map<std::string, std::string> fields;

	handler_data.cgi->buffer_out.push_back('\0');
	std::stringstream buffer_stream(handler_data.cgi->buffer_out.data());
	parse_header_fields(fields, handler_data.cgi->buffer_out, buffer_stream);

	auto it= fields.find("status");
	if (it != fields.end()) handler_data.current_response.set_status_code(it->second.substr(0, it->second.find_first_of(' ')));

	it = fields.find("content-type");
	if (it != fields.end()) handler_data.current_response.content_type = it->second;

	it = fields.find("content-length");
	if (it != fields.end()) handler_data.current_response.content_length = it->second;
	else
	{
		handler_data.current_request.fields["connection"] = "close";
		handler_data.current_response.content_length.clear();
	}

	if (!handler_data.current_response.status_code.empty())
		handler_data.cgi->buffer_out.clear();
}

void Connection::new_response_redirect(Server const& server, Location const& loc)
{
	std::cout << "Connection::new_response_redirect" << std::endl;

	handler_data.current_response.add_http_header("location", server.get_redirection(loc));
	handler_data.current_response.set_status_code("301");
	handler_data.current_response.add_http_header("content-length", "0");
}

void Connection::new_response_delete(Server const& server, Location const& loc)
{
	std::cout << "Connection::new_response_delete" << std::endl;
}

void Connection::continue_response(pollable_map_t& fd_map)
{
	if (handler_data.cgi != nullptr)
	{
		if (!handler_data.cgi->buffer_out.empty())
		{
			ssize_t send_data = data::send(socket_fd, handler_data.cgi->buffer_out);
			if (send_data != handler_data.cgi->buffer_out.size())
			{
				// TODO: Error-code. Something went wrong, can't write full size
				std::cerr << "Can't write full buffer to CGI";
			}
			else
				handler_data.cgi->buffer_out.clear();
		}

		int wstatus;
		int rpid = waitpid(handler_data.cgi->get_pid(), &wstatus, WNOHANG);
		if (rpid > 0)
		{
			fd_map.erase(handler_data.cgi->get_in_fd());
			fd_map.erase(handler_data.cgi->get_out_fd());
			delete handler_data.cgi;
			handler_data.cgi = nullptr;
			// TODO: check to keep
			std::cout << "CGI finished execution, exitcode: " << WEXITSTATUS(wstatus) << std::endl;
			state = CLOSE; // Close is default unless keep-alive
			return ;
			handler_data.current_request.fields["connection"] = "close";
			// 	state = READY_TO_READ;
		}
	}
	if (!handler_data.custom_page.empty())
	{
		ssize_t send_data = data::send(socket_fd, handler_data.custom_page);
		if (static_cast<size_t>(send_data) != handler_data.custom_page.size())
			std::cerr << "send_data of custom_page == " << send_data << std::endl;
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
		handler_data.custom_page.clear();
		return ;
	}
	if (handler_data.file.bad())
		return ;

	if (!data::send_file(socket_fd, handler_data.file, MAX_SEND_BUFFER_SIZE))
	{
		handler_data.file.close();
		handler_data.file.clear();
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
		return ;
	}
	reset_time_remaining();
}

// GETTERS
Request const& Connection::get_last_request(void) const { return last_request; }
Response const& Connection::get_last_response(void) const { return last_response; }
Connection::State Connection::get_state(void) const { return state; }

sockfd_t Connection::get_fd(void) const
{
	return (socket_fd);
}

bool Connection::should_destroy(void) const { return state == CLOSE && handler_data.cgi == nullptr; }

std::string Connection::get_ip(void) const
{
	char* cstr = inet_ntoa(reinterpret_cast<addr_in_t const*>(&address)->sin_addr);
	std::string as_str(cstr);
	return (as_str);
}

} // namespace webserv
