#ifndef COMMAND_H
# define COMMAND_H

# include "Core.h"

# include <functional>

namespace webserv {

class Command
{
	public:
	using function_t = std::function<void(std::ostream&, std::string)>;

	Command(std::string const& cmd);
	Command(std::string const& cmd, function_t func);
	Command(Command&& cmd);

	~Command();
	Command& add_subcommand(Command scmd);

	static Command* find(std::string& str);
	void run(std::ostream& out, std::string arguments);

	static void add_command(Command cmd);
	static void run_thread(void);

	private:
	Command* find_impl(std::stringstream& line_stream);

	public:
	std::string name;

	private:
	static std::unordered_map<std::string, Command> s_commands;

	function_t func;
	std::unordered_map<std::string, Command> subcommands;
};

void terminal_setup(void);

} // webserv

#endif // COMMAND_H