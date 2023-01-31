#ifndef COMMAND_H
# define COMMAND_H

# include "Core.h"

# include <functional>

namespace webserv {

class Command
{
	public:
	Command(std::string const& cmd);
	Command(std::string const& cmd, std::function<void()> func);
	Command(Command&& cmd);

	~Command();
	Command& add_subcommand(Command scmd);

	static Command* find(std::string str);
	void run();

	static void add_command(Command cmd);
	static void run_thread(void);

	static char** completion(const char * text , int start,  int end);

	private:
	Command* find_impl(std::stringstream& line_stream);

	public:
	std::string name;

	private:
	static std::unordered_map<std::string, Command> s_commands;

	std::function<void()> func;
	std::unordered_map<std::string, Command> subcommands;
};

void terminal_setup(void);

} // webserv

#endif // COMMAND_H