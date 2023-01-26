#ifndef RESPONSE_H
# define RESPONSE_H

# include "Core.h"

namespace webserv {

struct Response
{
	size_t size;
	char* data;
};

} // webserv

#endif // RESPONSE_H
