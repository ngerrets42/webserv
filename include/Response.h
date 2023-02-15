#ifndef RESPONSE_H
# define RESPONSE_H

# include "Core.h"
# include <ctime>

namespace webserv {

class Response{

	private: 
		//contains the supported status code and their corresponding messages
		static std::unordered_map<std::string, std::string> const& status_code_messages; 
		//initialize the static variable status_code_messages
		static std::unordered_map<std::string, std::string> const& init_status_messages(void);

	public:
		std::string http_version;	//default is HTTP/1.1
		std::string status_code;	//the status code of the response
		std::string reason; 		//the message that correspond with the status code
		std::string date;			//the date at which the response was ready
		std::string server;			//the name of the webserver in this case webserv
		std::string content_type;	//content type of the response
		std::string content_length; //content length in bytes
		//std::string body; //probably best that body is not a string object maybe do send the body seperately

		Response(void);
		~Response(void);

		void	set_date(void);
		void	set_default_values(void);
		bool	set_status_code(std::string const& response_code);
		std::string get_response(void);
		std::string get_response_const(void) const;

};

} //namespace webserv

#endif