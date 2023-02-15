#include "Response.h"
namespace webserv {

std::unordered_map<std::string, std::string> const& Response::status_code_messages = Response::init_status_messages();

std::unordered_map<std::string, std::string> const& Response::init_status_messages(void){
	static std::unordered_map<std::string , std::string> const messages
	{
		{"200", "OK"},
		{"201", "Created"},
		{"300", "Multiple Choices"},
		{"301", "Moved Permanently"},
		{"302", "Found"},
		{"303", "See Other"},
		{"307", "Temporary Redirect"},
		{"308", "Permanent Redirect"},
		{"400", "Bad Request"},
		{"403", "Forbidden"},
		{"404", "Not Found"},
		{"405", "Method Not Allowed"},
		{"408", "Request Timeout"},
		{"411", "Length Required"},
		{"413", "Payload Too Large"},
		{"414", "URI Too Long"},
		{"415", "Unsupported Media Type"},
		{"421", "Request Header Fields Too Large"},
		{"500", "Internal Server Error"},
		{"501", "Not Implemented"},
		{"502", "Bad Gateway"},
		{"503", "Service Unavailable"},
		{"504", "Gateway Timeout"},
		{"505", "HTTP Version Not Supported"}
	};
	return messages;
}

//constructor
Response::Response(void){
	set_default_values();
}

//destructor
Response::~Response(void){};

//sets the default values for a response
//in this case the http_version and the server application
void	Response::set_default_values(void){
	http_version = "HTTP/1.1";
	server = "webserv";
}

void	Response::set_date(void){
	time_t		rawtime;
	struct tm *	timeinfo;
	char 		buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime (buffer, 80, "%a, %d %b %Y %T %Z", timeinfo);
	date = buffer;
}

//will set the status code of the response and the message with that status code. 
//Return	true if it status code is found
//			false if the status code is not found
bool	Response::set_status_code(std::string const& response_code){
	if (status_code_messages.find(response_code) == status_code_messages.end())
		return false;
	status_code = response_code;
	reason = status_code_messages.find(response_code)->second;
	return true;
}

std::string Response::get_response(void){
	std::string response;

	//set the date of the response
	set_date();
	//build status-line
	response = http_version + " " + status_code + " " + reason + "\r\n";
	//http headers
	response += "content-length: " + content_length + "\r\n";
	// response += "content-type: " + content_type + "\r\n";
	response += "Server: " + server + "\r\n";
	response += "Date: " + date + "\r\n";
	
	response += "\r\n";
	return response;
}

std::string Response::get_response_const(void) const {
	std::string response;

	//build status-line
	response = http_version + " " + status_code + " " + reason + "\r\n";
	//http headers
	response += "content-length: " + content_length + "\r\n";
	response += "Server: " + server + "\r\n";
	response += "Date: " + date + "\r\n";
	
	response += "\r\n";
	return response;
}

} //namespace webserv