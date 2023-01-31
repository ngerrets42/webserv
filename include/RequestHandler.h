#ifndef REQUEST_HANDLER_H
# define REQUEST_HANDLER_H

# include "Core.h"

# include "Request.h"

namespace webserv {

class RequestHandler
{
	public:
	static void async();

	private:
	RequestHandler();

	private:
	static RequestHandler s_request_handler;
};


} // webserv

#endif // REQUEST_HANDLER_H
