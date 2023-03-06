#include "Connection.h"
#include "Core.h"
#include "Request.h"
#include "Socket.h"
#include "cgi.h"
#include "data.h"
#include "html.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <sstream>
#include <sys/_types/_ssize_t.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

namespace webserv {

#define MAX_SEND_BUFFER_SIZE 8192

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

// Getters
Request const& Connection::get_last_request(void) const { return last_request; }
Response const& Connection::get_last_response(void) const { return last_response; }
Connection::State Connection::get_state(void) const { return state; }

std::string Connection::get_ip(void) const
{
	char* cstr = inet_ntoa(reinterpret_cast<addr_in_t const*>(&address)->sin_addr);
	std::string as_str(cstr);
	return (as_str);
}

// POLL
void Connection::on_pollin(Socket& socket, sockfd_t fd)
{
	if (fd != socket_fd)
	{
		cgi_pollin();
		return ;
	}
	// Receive request OR continue receiving in case of POST
	switch (state)
	{
		case READY_TO_READ: new_request(); break;
		case READING: continue_request(); break;
		default: return;
	}	
}

void Connection::on_pollout(Socket& socket, sockfd_t fd)
{
	if (fd != socket_fd)
	{
		cgi_pollout();
		return ;
	}
	// Send response OR continue sending response
	switch (state)
	{
		case READY_TO_WRITE: new_response(socket); break;
		case WRITING: continue_response(); break;
		default: return;
	}
}

void Connection::new_request_post(void)
{
	//setting up the pipes
	pipe(handler_data.pipes.in);
	pipe(handler_data.pipes.out);

	handler_data.pid = fork();
	if(handler_data.pid < 0)
	{
		// TODO: fork error
		// Fork unavailable, therefore server error
		std::cerr << "fork not available: " << strerror(errno);
		// Response builder should check pid and send error-page
		return ;
	}

	// Child process directly turns into the CGI and becomes unavailable until completed
	if(handler_data.pid == 0) // child process
	{
		// Handle handler_data.pipes
		dup2(handler_data.pipes.in[0], STDIN_FILENO); close(handler_data.pipes.in[0]); close(handler_data.pipes.in[1]);
		dup2(handler_data.pipes.out[1], STDOUT_FILENO); close(handler_data.pipes.out[1]); close(handler_data.pipes.out[0]);

		// Build cgi-environment
		std::vector<std::string> env = cgi::env_init();
		cgi::env_set_value(env, "REMOTE_USER", "hman");
		cgi::env_set_value(env, "CONTENT_LENGTH",handler_data.current_request.fields["content-length"]);
		cgi::env_set_value(env, "CONTENT_TYPE", handler_data.current_request.fields["content-type"]);
		// cgi::env_set_value(env, "SCRIPT_FILENAME", "cgi-bin/handle_form.php");
		cgi::env_set_value(env, "REQUEST_METHOD", "POST");

		char* env_array[env.size() + 1];
		cgi::env_to_string_array(env_array, env);

		// Build argv
		std::vector<char*> exec_argv;
		std::string cgi_exec = "python3"; //loc.get_cgi(handler_data.current_request.path);

		if (cgi_exec.empty())
		{
			// TODO: Error
		}

		exec_argv.push_back(strdup(cgi_exec.c_str()));
		exec_argv.push_back(strdup("cgi-bin/handle_form.php"));
		exec_argv.push_back(NULL);

		// Actual execution
		if(execve(exec_argv[0], exec_argv.data(), env_array) != 0)
		{
			// TODO: Server error, send error-page (We are in child though, not that easy)
			std::cerr << "execve: Error" << std::endl;
			for (auto* p : exec_argv)
				free(p);
		}
	}
	
	if (handler_data.pid > 0) //parent processs
	{
		fcntl(handler_data.pipes.in[1], F_SETFL, O_NONBLOCK);
		fcntl(handler_data.pipes.out[0], F_SETFL, O_NONBLOCK);

		close(handler_data.pipes.in[0]);

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

		// TODO: SET READY TO WRITE TO CGI

		//close(handler_data.pipes.in[1]);
		
		close(handler_data.pipes.out[1]);
	}
}

// Request building
void Connection::new_request(void)
{
	state = READING;
	handler_data = HandlerData();
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
	});
	handler_data.current_request = request_build(handler_data.buffer);

	// Build the CGI
	if (handler_data.current_request.type == POST)
		new_request_post();
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
	handler_data.buffer = data::receive(socket_fd, HTTP_HEADER_BUFFER_SIZE, [&]{
		this->state = CLOSE;
	});

	handler_data.received_size += handler_data.buffer.size();
	std::cout << "receiving data " << handler_data.received_size << '/' << handler_data.content_size;

	// Write to CGI
	// std::cout << "Writing buffer to CGI: {" << (char*)handler_data.buffer.data() << "}" << std::endl;
	// ssize_t write_size = write(handler_data.pipes.in[1], handler_data.buffer.data(), handler_data.buffer.size());
	// if (write_size != static_cast<ssize_t>(handler_data.buffer.size()))
	// {
	// 	// TODO: Error-code. Something went wrong, can't write full size
	// 	std::cerr << "Can't write full buffer to CGI";
	// }

	// if (state == CLOSE || handler_data.received_size >= handler_data.content_size)
	// {
	// 	// Close the pipe when we are done receiving and writing the full body
	// 	close(handler_data.pipes.in[1]);
	// }
	// else state = READY_TO_WRITE;
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
void Connection::new_response(Socket& socket)
{
	state = WRITING;
	handler_data.current_response = Response();

	// Get the Server from host
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
	if (handler_data.current_response.content_length.empty())
		handler_data.current_response.content_length = "0";
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

	continue_response();
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

	if (handler_data.buffer.empty())
	{
		state = READY_TO_WRITE;
		return ;
	}

	handler_data.current_response.content_length = "0";
	handler_data.current_response.content_type = "text/plain";

	// parse into request
	std::unordered_map<std::string, std::string> fields;
	std::stringstream buffer_stream(handler_data.buffer.data());
	parse_header_fields(fields, handler_data.buffer, buffer_stream);

	// TODO: read "Status" header-field

	auto it = fields.find("content-type");
	if (it != fields.end()) handler_data.current_response.content_type = it->second;
	it = fields.find("content-length");
	if (it != fields.end()) handler_data.current_response.content_length = it->second;
	else
		handler_data.current_response.content_length = std::to_string(handler_data.buffer.size());
}

void Connection::new_response_delete(Server const& server, Location const& loc)
{

}

void Connection::continue_response(void)
{
	if (handler_data.current_request.type == POST)
	{
		if (handler_data.buffer.empty())
			return ;

		ssize_t send_data = data::send(socket_fd, handler_data.buffer);
		if (send_data != handler_data.buffer.size())
		{
		// TODO: Error-code. Something went wrong, can't write full size
		std::cerr << "Can't write full buffer to CGI";
		}

		handler_data.buffer.clear();

		int wstatus;
		int rpid = waitpid(handler_data.pid, &wstatus, WNOHANG);
		if (rpid <= 0)
		{
			close(handler_data.pipes.out[0]);
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

void Connection::cgi_pollin(void)
{
	handler_data.buffer.resize(MAX_SEND_BUFFER_SIZE);
	ssize_t read_size = read(handler_data.pipes.out[0], handler_data.buffer.data(), MAX_SEND_BUFFER_SIZE);
	if (read_size < 0)
		handler_data.buffer.clear();
	else if (static_cast<size_t>(read_size) != MAX_SEND_BUFFER_SIZE)
		handler_data.buffer.resize(read_size);
}

void Connection::cgi_pollout(void)
{
	// Write body buffer to CGI
	ssize_t write_size = write(handler_data.pipes.in[1], handler_data.buffer.data(), handler_data.buffer.size());
	if (write_size != static_cast<ssize_t>(handler_data.buffer.size()))
	{
		// TODO: Error-code. Something went wrong, can't write full size
		std::cerr << "Can't write full buffer to CGI";
	}

	if (state == CLOSE || handler_data.received_size >= handler_data.content_size)
	{
		// Close the pipe when we are done receiving and writing the full body
		close(handler_data.pipes.in[1]);
	}
	else state = READY_TO_WRITE;

	handler_data.buffer.clear();
}

} // namespace webserv
