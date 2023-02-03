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

Command* Command::find_impl(std::stringstream& line_stream)
{
	std::string key;

	std::cout << "find_impl() - tellg before: " << line_stream.tellg() << std::endl;
	line_stream >> std::ws >> key;

	std::cout << "find_impl() key: " << key << std::endl;
	if (key.length() > 0)
	{
		if (subcommands.find(key) != subcommands.end())
			return subcommands.at(key)->find_impl(line_stream);
		line_stream.seekg(static_cast<size_t>(line_stream.tellg()) - key.length() - 1);
		std::cout << "find_impl() - tellg: " << line_stream.tellg() << std::endl;
	}
	return (this);
}

Command* Command::find(std::string& str)
{
	std::stringstream line_stream(str);
	std::string key;

	line_stream >> std::ws >> key;
	if (s_commands.find(key) != s_commands.end())
	{
		Command* cmd = s_commands.at(key)->find_impl(line_stream);
		if (line_stream.eof())
			str.clear();
		else if (line_stream.tellg() == -1)
			str = str.substr(key.length() + 1);
		else
			str = str.substr(static_cast<size_t>(line_stream.tellg()) + 1);
		return cmd;
	}
	return (nullptr);
}

void Command::run(std::ostream& out, std::string arguments)
{
	func(out, arguments);
}

} // webserv
