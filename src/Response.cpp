#include "Response.h"
namespace webserv {

std::unordered_map<std::string, std::string> Response::status_code_messages = Response::init_status_messages();

std::unordered_map<std::string, std::string> Response::init_status_messages(void){
	std::unordered_map<std::string , std::string> messages;
	messages["200"] = "OK";
	messages["201"] = "Created";
	messages["300"] = "Multiple Choices";
	messages["301"] = "Moved Permanently";
	messages["302"] = "Found";
	messages["303"] = "See Other";
	messages["307"] = "Temporary Redirect";
	messages["308"] = "Permanent Redirect";
	messages["400"] = "Bad Request";
	messages["403"] = "Forbidden";
	messages["404"] = "Not Found";
	messages["405"] = "Method Not Allowed";
	messages["408"] = "Request Timeout";
	messages["411"] = "Length Required";
	messages["413"] = "Payload Too Large";
	messages["414"] = "URI Too Long";
	messages["415"] = "Unsupported Media Type";
	messages["421"] = "Request Header Fields Too Large";
	messages["500"] = "Internal Server Error";
	messages["501"] = "Not Implemented";
	messages["502"] = "Bad Gateway";
	messages["503"] = "Service Unavailable";
	messages["504"] = "Gateway Timeout";
	messages["505"] = "HTTP Version Not Supported";
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
bool	Response::set_status_code(std::string response_code){
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
	response = http_version + " " + status_code + " " + reason + "\n";
	//http headers
	response += "server: " + server + "\n";
	response += "date: " + date + "\n";

	return response;
}

} //namespace webserv