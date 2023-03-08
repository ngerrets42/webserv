#include "CGI.h"

namespace webserv {

namespace env
{

	std::vector<std::string> initialize(void)
	{
		static std::vector<std::string> env = {
			"AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
			"PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
			"REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
			"SCRIPT_NAME", "SERVER_NAME", "SERVER_PROTOCOL", "SERVER_SOFTWARE"
		};

		for(auto& s : env) s += '=';
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

CGI::CGI(std::vector<std::string>& env, Server& server, Location& loc)
{
	//setting up the pipes
	pipe(pipes.in);
	pipe(pipes.out);

	pid = fork();
	if(pid < 0)
	{
		// TODO: fork error
		// Fork unavailable, therefore server error
		std::cerr << "fork not available: " << strerror(errno);
		// Response builder should check pid and send error-page
		return ;
	}

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
		std::string cgi_exec = "/usr/local/bin/python3"; //server.get_cgi(handler_data.current_request.path, loc);

		if (cgi_exec.empty())
		{
			// TODO: Error
		}

		// Build argv
		exec_argv.push_back(strdup(cgi_exec.c_str()));
		exec_argv.push_back(strdup("cgi-bin/upload_file.py"));
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
	close(pipes.in[1]);
	close(pipes.out[0]);
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
	if (fd == pipes.in[1])
		return (POLLOUT);
	return (POLLIN);
}

void CGI::on_pollin(pollable_map_t& fd_map)
{
	// READ FROM CGI
	std::cout << "Reading from CGI";

	buffer_out.resize(MAX_SEND_BUFFER_SIZE);
	ssize_t read_size = read(pipes.out[0], buffer_out.data(), MAX_SEND_BUFFER_SIZE);
	if (read_size < 0)
		buffer_out.clear();
	else if (static_cast<size_t>(read_size) != MAX_SEND_BUFFER_SIZE)
		buffer_out.resize(read_size);
	std::cout << ": " << buffer_out.size() << " Bytes read" << std::endl;
}
void CGI::on_pollout(pollable_map_t& fd_map)
{
	// WRITE TO CGI
	// std::cout << "Writing to CGI";
	if (buffer_in.empty())
	{
		// std::cout << ": Buffer empty";
		return ;
	}

	// Write body buffer to CGI
	ssize_t write_size = write(pipes.in[1], buffer_in.data(), buffer_in.size());
	if (write_size != static_cast<ssize_t>(buffer_in.size()))
	{
		// TODO: Error-code. Something went wrong, can't write full size
		std::cerr << ": Can't write full buffer to CGI";
	}
	std::cout << ": " << write_size << " Bytes written to CGI" << std::endl;

	if (write_size > 0)
		buffer_in.erase(buffer_in.begin(), buffer_in.begin() + write_size);
}

void CGI::on_pollhup(pollable_map_t& fd_map)
{
	std::cout << "CGI-POLLHUP";
}

} // namespace webserv
