#ifndef DATA_H
# define DATA_H

# include "Core.h"

namespace webserv {

namespace data
{
	std::vector<char> receive(sockfd_t fd, size_t max_size, std::function<void()> const& on_zero = nullptr);

	bool file_is_valid(std::string const& fpath);
	size_t get_file_size(std::string const& fpath);

	ssize_t send(sockfd_t fd, std::vector<char> const& buffer);
	ssize_t send(sockfd_t fd, std::string const& str);

	bool send_file(sockfd_t fd, std::ifstream& istream, size_t buffer_size);
} // namespace data

} // namespace webserv

#endif // DATA_H
