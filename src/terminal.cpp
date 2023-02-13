#include "Command.h"

// # include <termios.h>

namespace webserv {

// static void	remove_term_controlchars(void)
// {
// 	struct termios ctl = {};

// 	tcgetattr(STDIN_FILENO, &ctl);
// 	ctl.c_lflag &= ~(ECHOCTL);
// 	tcsetattr(STDIN_FILENO, TCSANOW, &ctl);
// }

// TODO: Remove
void Command::run_thread(void)
{
	while (true)
	{
		std::string str;
		std::getline(std::cin, str);
		Command* cmd = Command::find(str);
		if (cmd == nullptr)
		{
			std::cout << "Command doesn't exist" << std::endl;
			continue;
		}
		else
			cmd->run(std::cout, str);
	}
}

void terminal_setup(void)
{
	// remove_term_controlchars();
	
	std::thread readline_thread(Command::run_thread);
	readline_thread.detach();
}


} // webserv
