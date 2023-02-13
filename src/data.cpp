#include "data.h"

namespace webserv {

namespace data
{
	// Return a buffer of data that should contain the header of the request
	std::vector<char> receive(sockfd_t fd, size_t max_size, std::function<void()> on_zero)
	{
		std::vector<char> buffer(max_size);
		ssize_t recv_size = recv(fd, buffer.data(), max_size, 0);

		if (recv_size == 0)
		{
			if (on_zero) on_zero();
			return (std::vector<char>());
		}

		if (recv_size < 0)
			return (std::vector<char>());

		if (static_cast<size_t>(recv_size) != max_size)
			buffer.resize(recv_size);
		return buffer;
	}
} // data

} // webserv
