#include "Command.h"

namespace webserv {

std::unordered_map<std::string, Command::pointer> Command::s_commands;

Command::Command(std::string const& cmd)
:	name(cmd),
	func(nullptr) {}

Command::Command(std::string const& cmd, Command::function_t func)
:	name(cmd),
	func(func) {}

Command::Command(Command&& cmd)
:	name(std::move(cmd.name)),
	func(std::move(cmd.func)) {}

Command::~Command() {}

Command& Command::add_subcommand(Command* scmd)
{
	subcommands.emplace(scmd->name, pointer (scmd));
	return (*this);
}

void Command::add_command(Command* cmd)
{
	s_commands.emplace(cmd->name, pointer (cmd));
}

Command* Command::find_impl(std::stringstream& line_stream, std::stringstream& consumed_stream)
{
	std::string key;

	line_stream >> std::ws >> key;
	if (key.length() > 0)
	{
		if (subcommands.find(key) != subcommands.end())
		{
			std::string tmp;
			consumed_stream >> tmp;
			return subcommands.at(key)->find_impl(line_stream, consumed_stream);
		}
	}
	return (this);
}

Command* Command::find(std::string& str)
{
	std::stringstream line_stream(str);
	std::stringstream consumed_stream(str);
	std::string key;

	line_stream >> std::ws >> key;
	if (s_commands.find(key) != s_commands.end())
	{
		Command* cmd = s_commands.at(key)->find_impl(line_stream, consumed_stream);
		str.clear();
		std::string tmp;
		consumed_stream >> tmp;
		while (!consumed_stream.eof())
		{
			consumed_stream >> tmp;
			str += tmp;
			if (!consumed_stream.eof())
				str += " ";
		}
		return cmd;
	}
	return (nullptr);
}

void Command::run(std::ostream& out, std::string arguments)
{
	if (func) func(out, arguments);
}

} // webserv
