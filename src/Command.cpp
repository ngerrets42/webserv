#include "Command.h"

namespace webserv {

std::unordered_map<std::string, Command> Command::s_commands;

Command::Command(std::string const& cmd)
:	name(cmd),
	func(nullptr)
{
	
}

Command::Command(std::string const& cmd, std::function<void()> func)
:	name(cmd),
	func(func)
{

}

Command::Command(Command&& cmd)
:	name(std::move(cmd.name)),
	func(std::move(cmd.func)) {}

Command::~Command() {}

Command& Command::add_subcommand(Command scmd)
{
	subcommands.emplace(scmd.name, std::move(scmd));
	return (*this);
}

void Command::add_command(Command cmd)
{
	s_commands.emplace(cmd.name, std::move(cmd));
}

Command* Command::find_impl(std::stringstream& line_stream)
{
	std::string key;

	line_stream >> std::ws >> key;
	if (key.length() > 0)
	{
		if (subcommands.find(key) != subcommands.end())
			return subcommands.at(key).find_impl(line_stream);
		else
			return (nullptr);
	}
	return (this);
}

Command* Command::find(std::string str)
{
	std::stringstream line_stream(str);
	std::string key;

	line_stream >> std::ws >> key;
	if (s_commands.find(key) != s_commands.end())
		return s_commands.at(key).find_impl(line_stream);
	return (nullptr);
}

void Command::run()
{
	func();
}

} // webserv
