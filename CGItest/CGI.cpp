#include "CGI.h"
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <vector>

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

int main(int argc, char ** argv, char ** envp)
{
	if (argc != 2)
	{
		std::cout << "one argument needed" << std::endl;
		return (1);
	}


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
		set_value(env, "CONTENT_LENGTH","100");
		char * env_array[env.size() + 1];
		convert_to_string_array(env_array, env);
		if(execve(argv[1], &argv[1], env_array))
		{
			std::cout << "Error" << std::endl;
		}
	}
	else if(pid != 0) //parent processs
	{
		std::cout << "parent process" << pid << std::endl;
		close(inputpipefd[0]);
		write(inputpipefd[1],"hello haoafsd aldsjklasdjffjaklsfjkljfladksfjdsfklajsfkladsjkfa",64);
		close(inputpipefd[1]);
		close(outputpipefd[1]);
		int wstatus;
		waitpid(pid,&wstatus,WNOHANG);
		std::cout << "exitcode " << WEXITSTATUS(wstatus) << std::endl;
		char buff[1024] = {0};
		std::cout << "test" << std::endl;
		std::cout << "read bytes" << read(outputpipefd[0],buff,1024) << std::endl;
		std::cout << "printing output" << std::endl;
		std::cout<< buff << std::endl;
		close(outputpipefd[0]);
	}
}
