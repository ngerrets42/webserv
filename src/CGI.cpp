#include "CGI.h"
#include "Core.h"
#include "Pollable.h"
#include <cstring>
#include <stdexcept>
#include <unistd.h>

namespace webserv {

namespace env
{

	std::vector<std::string> initialize(void)
	{
		static std::vector<std::string> env = {
			"AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
			"PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
			"REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
			"SCRIPT_NAME", "SERVER_NAME", "SERVER_PROTOCOL", "SERVER_SOFTWARE",
			"UPLOAD_DIRECTORY"
		};

		if (env[0].back() != '=')
		{
			for(auto& s : env) s += '=';
		}
		return (env);
	}

	bool set_value(std::vector<std::string>& env, std::string const& var, std::string const& value)
	{
		for(size_t i = 0; i < env.size(); i ++) {
			if (env[i].substr(0,env[i].find('=')) == var)
			{
				env[i] = var + "=" + value;
				return true;
			}
		}
		return false;
	}

	void to_string_array(char ** env_array, std::vector<std::string> &env)
	{
		for(size_t i = 0; i < env.size(); i++)
		{
			env_array[i] = (char *) env[i].c_str();
		}
		env_array[env.size()] = NULL;
	}

	void print(std::vector<std::string> const& env){
		for(size_t i = 0; i < env.size(); i++) {
			std::cout << env[i] << std::endl;
		}
	}

} // namespace env

CGI::CGI(std::vector<std::string>& env, Server& server, Location& loc, std::string const& path) : destroy(false)
{
	std::cout << "Lauching new CGI" << std::endl;
	//setting up the pipes
	pipe(pipes.in);
	pipe(pipes.out);

	open_in = true;
	open_out = true;

	pid = fork();
	if(pid < 0)
		// Fork unavailable THROW, needs to be caught
		throw std::runtime_error(std::string {"CGI::CGI() failed to fork: "} + strerror(errno) );

	// Child process directly turns into the CGI and becomes unavailable until completed
	if(pid == 0) // child process
	{
		// Handle pipes
		dup2(pipes.in[0], STDIN_FILENO); close(pipes.in[0]); close(pipes.in[1]);
		dup2(pipes.out[1], STDOUT_FILENO); close(pipes.out[1]); close(pipes.out[0]);

		// Build char** out of array
		char* env_array[env.size() + 1];
		env::to_string_array(env_array, env);

		// Build argv
		std::vector<char*> exec_argv;

		std::string cgi_path = server.get_root(loc) + server.get_cgi(loc, path).first;
		std::string cgi_exec = cgi_path.substr(cgi_path.find_last_of('/') + 1);
		cgi_path = cgi_path.substr(0, cgi_path.find_last_of('/'));

		// Build argv
		exec_argv.push_back(strdup(cgi_exec.c_str()));
		exec_argv.push_back(NULL);

		std::cerr << "CGI (" << pipes.in[1] << ", " << pipes.out[0] << ") executing: " << exec_argv[0] << std::endl;

		// CHange directory
		if (chdir(cgi_path.c_str()) != 0)
		{
			std::cerr << strerror(errno) << std::endl;
			std::string status = "status: 500 Internal Server Error\r\n";
			std::cout << status << std::endl;
			exit(1);
		}

		// Actual execution
		if(execve(exec_argv[0], exec_argv.data(), env_array) != 0)
		{
			std::string status = "status: 500 Internal Server Error\r\n";
			std::cout << status << std::endl;
			std::cerr << "CGI::CGI() execve error" << std::endl;
			for (auto* p : exec_argv)
				free(p);
			exit(1);
		}
	}
	if (pid > 0) //parent process
	{
		fcntl(pipes.in[1], F_SETFL, O_NONBLOCK);
		fcntl(pipes.out[0], F_SETFL, O_NONBLOCK);

		close(pipes.in[0]);
		close(pipes.out[1]);
	}
}

CGI::~CGI()
{
	std::cerr << "deleting CGI (" << pipes.in[1] << ", " << pipes.out[0] << ')' << std::endl;
	close_pipes();
}

sockfd_t CGI::get_fd(void) const
{
	return (-1);
}

int CGI::get_pid(void) const
{
	return (pid);
}

int CGI::get_in_fd(void) const
{
	return (pipes.in[1]);
}

int CGI::get_out_fd(void) const
{
	return (pipes.out[0]);
}

short CGI::get_events(sockfd_t fd) const
{
	short events = 0;
	if (fd == pipes.out[0] && buffer_out.empty())
		events |= POLLIN;
	else if (fd == pipes.in[1] && !buffer_in.empty())
		events |= POLLOUT;
	return (events);
}

void CGI::on_pollin(pollable_map_t& fd_map)
{
	(void)fd_map;
	// READ FROM CGI
	std::cout << "CGI::on_pollin (" << pipes.in[1] << ", " << pipes.out[0] << ')' << std::endl;

	buffer_out.resize(MAX_SEND_BUFFER_SIZE);
	ssize_t read_size = read(pipes.out[0], buffer_out.data(), MAX_SEND_BUFFER_SIZE);
	if (read_size < 0)
		buffer_out.clear();
	else if (read_size == 0)
	{
		if (open_out)
		{
			close(pipes.out[0]);
			open_out = false;
		}
	}
	else if (static_cast<size_t>(read_size) != MAX_SEND_BUFFER_SIZE)
		buffer_out.resize(read_size);
	std::cout << ": " << buffer_out.size() << " Bytes read" << std::endl;
}

void CGI::on_pollout(pollable_map_t& fd_map)
{
	(void)fd_map;
	// std::cout << "CGI::on_pollout (" << pipes.in[1] << ", " << pipes.out[0] << ')' << std::endl;

	// Write body buffer to CGI
	ssize_t write_size = write(pipes.in[1], buffer_in.data(), buffer_in.size());
	if (write_size == 0)
	{
		if (open_in)
		{
			close(pipes.in[1]);
			open_in = false;
		}
	}
	else if (write_size < 0)
		return ;
	// if (write_size != static_cast<ssize_t>(buffer_in.size()))
	// 	std::cerr << "Warning: Can't write full buffer to CGI, trying again next iteration" << std::endl;

	if (write_size > 0)
		buffer_in.erase(buffer_in.begin(), buffer_in.begin() + write_size);
}

void CGI::close_in(void)
{
	if (!open_in)
		return;
	close(pipes.in[1]);
	open_in = false;
}

void CGI::close_pipes(void)
{
	if (open_in)
	{
		close(pipes.in[1]);
		open_in = false;
	}
	if (open_out)
	{
		close(pipes.out[0]);
		open_out = false;
	}
}

void CGI::on_pollhup(pollable_map_t& fd_map, sockfd_t fd)
{
	std::cout << "CGI::on_pollhup (" << pipes.in[1] << ", " << pipes.out[0] << ')' << std::endl;
	// destroy = true;
	fd_map.erase(fd);
	if (fd == pipes.in[1])
	{
		if (open_in)
			close(fd);
		open_in = false;
	}
	if (fd == pipes.out[0])
	{
		if (open_out)
			close(fd);
		open_out = false;
	}
}

bool CGI::should_destroy(void) const
{
	return (false);
}

} // namespace webserv
