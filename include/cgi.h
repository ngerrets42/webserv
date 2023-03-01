#ifndef CGI_H

# include "Core.h"

namespace webserv {

	namespace cgi
	{
		std::vector<std::string> env_init(void);
		bool env_set_value(std::vector<std::string>& env, std::string const& var, std::string const& value);
		void env_to_string_array(char ** env_array, std::vector<std::string> &env);
		void env_print(std::vector<std::string> const& env);
	} // namespace cgi

} // namespace webserv

#endif // CGI_H
