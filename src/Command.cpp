#include "Command.h"

namespace webserv {

std::unordered_map<std::string, Command::pointer> Command::s_commands;

Command::Command(std::string const& cmd)
:	name(cmd),
	func(nullptr)
{
	
}

Command::Command(std::string const& cmd, Command::function_t func)
:	name(cmd),
	func(func)
{

}

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

Command* Command::find_impl(std::stringstream& line_stream, std::stringstream& cons_stream)
{
	std::string key;

	line_stream >> std::ws >> key;
	if (key.length() > 0)
	{
		if (subcommands.find(key) != subcommands.end())
		{
			std::string tmp;
			cons_stream >> tmp;
			return subcommands.at(key)->find_impl(line_stream, cons_stream);
		}
	}
	return (this);
}

Command* Command::find(std::string& str)
{
	std::stringstream line_stream(str);
	std::stringstream cons_stream(str);
	std::string key;

	line_stream >> std::ws >> key;
	if (s_commands.find(key) != s_commands.end())
	{
		Command* cmd = s_commands.at(key)->find_impl(line_stream, cons_stream);
		str.clear();
		std::string tmp;
		cons_stream >> tmp;
		while (!cons_stream.eof())
		{
			cons_stream >> tmp;
			str += tmp;
			if (!cons_stream.eof())
				str += " ";
		}
		return cmd;
	}
	return (nullptr);
}

void Command::run(std::ostream& out, std::string arguments)
{
	func(out, arguments);
}

} // webserv
