#include "Command.h"

# include <termios.h>
# include <readline/history.h>
# include <readline/readline.h>

namespace webserv {

static void	remove_term_controlchars(void)
{
	struct termios	ctl;

	tcgetattr(STDIN_FILENO, &ctl);
	ctl.c_lflag &= ~(ECHOCTL);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctl);
}

void Command::run_thread(void)
{
	std::vector<char*> history;
	while (true)
	{
		char* cptr = readline("> ");
		if (!cptr)
			continue ;
		
		std::string str(cptr);
		Command* cmd = Command::find(str);
		if (cmd == nullptr)
		{
			std::cout << "Command doesn't exist" << std::endl;
			free(cptr);
			continue;
		}
		history.push_back(cptr);
		add_history(cptr);
	}
}

// char* completion_generator(const char* text, int state)
// {
	

char** Command::completion(const char * text , int start,  int end)
{
	std::vector<char*> matches;

	std::string str(text);
	Command* cmd = Command::find(str);

	std::cout << "completion called" << std::endl;

	if (cmd == nullptr)
		return (NULL);

	for (auto& pair : cmd->subcommands)
	{
		char* cpy = strdup(pair.first.c_str());
		matches.push_back(cpy);
	}
	matches.push_back(NULL);
	return (matches.data());
}

void terminal_setup(void)
{
	remove_term_controlchars();
	rl_attempted_completion_function = Command::completion;
	
	std::thread readline_thread(Command::run_thread);
	readline_thread.detach();
}


} // webserv
