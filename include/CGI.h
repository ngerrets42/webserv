#ifndef CGI_H
# define CGI_H

# include "Core.h"
# include "Pollable.h"
# include "Server.h"

namespace webserv {

	namespace env
	{
		std::vector<std::string> initialize(void);
		bool set_value(std::vector<std::string>& env, std::string const& var, std::string const& value);
		void to_string_array(char ** env_array, std::vector<std::string> &env);
		void print(std::vector<std::string> const& env);
	} // namespace env

	class CGI : public Pollable
	{
		public:
		CGI(std::vector<std::string>& env, Server& server, Location& loc);
		virtual ~CGI();

		virtual sockfd_t get_fd(void) const override;

		int get_in_fd(void) const;
		int get_out_fd(void) const;

		int get_pid(void) const;

		virtual short get_events(sockfd_t fd) const override;

		protected:
		virtual void on_pollin(pollable_map_t& fd_map) override; // Read from the CGI to Server
		virtual void on_pollout(pollable_map_t& fd_map) override; // Write from Server to CGI
		virtual void on_pollhup(pollable_map_t& fd_map) override;

		private:
		int pid;

		struct s_pipes
		{
			int in[2];
			int out[2];
		} pipes;

		public:
		std::vector<char> buffer_in; // Into the CGI
		std::vector<char> buffer_out; // From the CGI
	};

} // namespace webserv

#endif // CGI_H
