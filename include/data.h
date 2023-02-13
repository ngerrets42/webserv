#ifndef DATA_H
# define DATA_H

# include "Core.h"

namespace webserv {

namespace data
{
	std::vector<char> receive(sockfd_t fd, size_t max_size, std::function<void()> on_zero = nullptr);
}

} // webserv

#endif // DATA_H
