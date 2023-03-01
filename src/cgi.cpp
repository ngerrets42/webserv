#include "cgi.h"

namespace webserv {

namespace cgi
{

	std::vector<std::string> env_init(void)
	{
		static std::string meta_vars[] = {
			"AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
			"PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
			"REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
			"SCRIPT_NAME", "SERVER_NAME", "SERVER_PROTOCOL", "SERVER_SOFTWARE",
			"SCRIPT_FILENAME"
		};

		std::vector<std::string> env(sizeof(meta_vars));
		for(auto& s : meta_vars)
			env.push_back(s + "=");
		return (env);
	}

	bool	env_set_value(std::vector<std::string>& env, std::string const& var, std::string const& value)
	{
		for(size_t i = 0; i < env.size(); i ++) {
			if (env[i].substr(0,env[i].find('=')) == var)
			{
				env[i] = var + "=" + value;
				return true;
			}
		}
		return false;
	}

	void env_to_string_array(char ** env_array, std::vector<std::string> &env)
	{
		for(size_t i = 0; i < env.size(); i++)
		{
			env_array[i] = (char *) env[i].c_str();
		}
		env_array[env.size()] = NULL;
	}

	void env_print(std::vector<std::string> const& env){
		for(size_t i = 0; i < env.size(); i++) {
			std::cout << env[i] << std::endl;
		}
	}

} // namespace cgi

} // namespace webserv
