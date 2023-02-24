#include "Connection.h"
#include "Request.h"
#include "Socket.h"
#include "data.h"
#include "html.h"

#include <cstring>
#include <sys/_types/_ssize_t.h>
#include <unistd.h>
#include <cstdlib>

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

///
/// TEST CGI
///


bool	set_value(std::vector<std::string> & env, std::string const & var, std::string const & value)
{
	for(int i = 0; i < env.size(); i ++){
		if (env[i].substr(0,env[i].find('=')).compare(var) == 0)
		{
			env[i] = var + "=" + value;
			return true;
		}
	}
	return false;
}

void	init_env(std::vector<std::string> & env)
{
	const int AMOUNT_META_VARS = 16;

	std::string meta_vars[AMOUNT_META_VARS] = {
		"AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
		"PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
		"REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
		"SCRIPT_NAME", "SERVER_NAME", "SERVER_PROTOCOL", "SERVER_SOFTWARE"
	};

	for(int i = 0; i < AMOUNT_META_VARS; i++)
	{
		env.push_back(meta_vars[i] + "=");
	}
}

void	convert_to_string_array(char ** env_array, std::vector<std::string> &env)
{
	for(int i = 0; i < env.size(); i++)
	{
		env_array[i] = (char *) env[i].c_str();
	}
	env_array[env.size()] = NULL;
}

void	print_env(std::vector<std::string> & env){
	for(int i = 0; i < env.size(); i++){
		std::cout << env[i] << std::endl;
	}
}

/// END TEST CGI



void Connection::new_response(Socket& socket)
{
	state = WRITING;
	handler_data.current_response = Response();

	// Get the Server from host
	Server& server = socket.get_server(handler_data.current_request.fields["host"]);
	Location loc = server.get_location(handler_data.current_request.path);

	// std::cout << " LOCPATH: " << loc.path << "AUTOINDEX: " << loc.autoindex.first << '|' << loc.autoindex.second << ", INDEX: " << loc.index;

	std::string const& root = server.get_root(loc);

	std::string fpath = root + handler_data.current_request.path;
	
	handler_data.current_response.set_status_code("200");

	if (handler_data.current_request.type == GET)
	{

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

		if (handler_data.current_request.fields["connection"] == "keep-alive")
			handler_data.current_response.add_http_header("connection", "keep-alive");
		else
			handler_data.current_response.add_http_header("connection", "close");

		if (!handler_data.current_response.content_type.empty())
			handler_data.current_response.add_http_header("content-type", handler_data.current_response.content_type);
	}

	else if (handler_data.current_request.type == POST)
	{

		// TODO: Don't hardcode

		//setting up the pipes
		int inputpipefd[2];
		int outputpipefd[2];
		pipe(inputpipefd);
		pipe(outputpipefd);

		int pid = fork();
		if(pid < 0)
		{
			exit(EXIT_FAILURE);
		}
		else if(pid == 0) //child process
		{
			dup2(inputpipefd[0],0);
			close(inputpipefd[0]);
			close(inputpipefd[1]);
			dup2(outputpipefd[1], 1);
			close(outputpipefd[1]);
			close(outputpipefd[0]);

			std::vector<std::string> env;
			init_env(env);
			set_value(env, "REMOTE_USER", "hman");
			set_value(env, "CONTENT_LENGTH",handler_data.current_request.fields["content-length"]);
			set_value(env, "CONTENT_TYPE", handler_data.current_request.fields["content-type"]);
			set_value(env, "REQUEST_METHOD", "POST");

			char * env_array[env.size() + 1];
			convert_to_string_array(env_array, env);


			std::vector<char*> exec_argv;

			// exec_argv.push_back(strdup("/usr/local/bin/python3"));
			// exec_argv.push_back(strdup("cgi-bin/upload_file.py"));
			exec_argv.push_back(strdup("/usr/bin/php"));
			exec_argv.push_back(strdup("cgi-bin/upload_file.php"));
			exec_argv.push_back(NULL);

			if(execve(exec_argv[0], exec_argv.data(), env_array) != 0)
			{
				std::cerr << "execve: Error" << std::endl;
				for (auto* p : exec_argv)
					free(p);
			}
		}
		else if(pid != 0) //parent processs
		{
			std::cout << "parent process" << pid << std::endl;
			close(inputpipefd[0]);

			std::cout << "Writing buffer to CGI: {" << (char*)handler_data.buffer.data() << "}" << std::endl;
			write(inputpipefd[1], handler_data.buffer.data(), handler_data.buffer.size());

			close(inputpipefd[1]);
			close(outputpipefd[1]);
			int wstatus;
			waitpid(pid,&wstatus, 0); // DO HANG
			std::cout << "exitcode " << WEXITSTATUS(wstatus) << std::endl;
			
			// Keep reading and store in buffer
			char buff[1024] = {0};

			ssize_t read_size;
			while ((read_size = read(outputpipefd[0], buff, 1024)) > 0)
			{
				handler_data.custom_page.append(buff, read_size);
			}
			close(outputpipefd[0]);

			handler_data.custom_page.append("\r\n");
			handler_data.current_response.content_length = std::to_string(handler_data.custom_page.size());
			handler_data.current_response.content_type = "text/html";

			handler_data.current_response.add_http_header("content-length",handler_data.current_response.content_length);
			handler_data.current_response.add_http_header("content-type", handler_data.current_response.content_type);

			std::cout << "CUSTOM_PAGE: {" << (const char*)handler_data.custom_page.data() << '}' << std::endl;

			std::cout << "DONE" << std::endl;
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
		ssize_t send_data = data::send(socket_fd, handler_data.custom_page);
		if (send_data != handler_data.custom_page.size())
			std::cerr << "send_data of custom_page == " << send_data << std::endl;
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
	}
	else if (!data::send_file(socket_fd, handler_data.file, MAX_SEND_BUFFER_SIZE))
	{
		state = CLOSE; // Close is default unless keep-alive
		if (handler_data.current_request.fields["connection"] == "keep-alive")
			state = READY_TO_READ;
	}
}

} // namespace webserv
