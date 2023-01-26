#ifndef RESPONSE_H
# define RESPONSE_H

# include "Core.h"

namespace webserv {

struct Response
{
	std::string header;
	std::string file_path;
	size_t file_size;
};

} // webserv

#endif // RESPONSE_H
