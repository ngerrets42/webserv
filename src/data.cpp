#include "data.h"

namespace webserv {

namespace data
{
	// Return a buffer of data that should contain the header of the request
	std::vector<char> receive(sockfd_t fd, size_t max_size, std::function<void()> const& on_zero)
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

	ssize_t send(sockfd_t fd, std::vector<char> const& buffer)
	{
		if (buffer.empty())
			return 0;

		ssize_t send_size = ::send(fd, buffer.data(), buffer.size(), 0);
		return (send_size);
	}

	ssize_t send(sockfd_t fd, std::string const& str)
	{
		if (str.empty())
			return 0;

		ssize_t send_size = ::send(fd, str.data(), str.length(), 0);
		return (send_size);
	}

	// Send a chunk of a file (ifstream)
	bool send_file(sockfd_t fd, std::ifstream& istream, size_t buffer_size)
	{
		if (!istream || istream.eof())
			return (false); // No more file left to conquer

		std::vector<char> buffer(buffer_size);
		istream.read( reinterpret_cast<char*>(buffer.data()) , buffer.size());

		if (istream.gcount() <= 0)
		{
			std::cerr << "istream.read() gcount = " << istream.gcount() << std::endl;
			return (false);
		}
		buffer.resize(istream.gcount());
		ssize_t send_size = send(fd, buffer);
		if (send_size < 0 || static_cast<size_t>(send_size) != buffer.size())
		{
			std::cerr << "send_size != buffer.size()" << std::endl;
			if (send_size < 0)
				return (false);

			std::cout << "send_file {\n" 
			<< "buffer.size(): " << buffer.size() << '\n'
			<< "send_size: " << send_size << '\n'
			<< "istream pos before: " << istream.tellg() << '\n';
		
			istream.seekg(istream.tellg() - static_cast<std::ifstream::pos_type>(buffer.size() - send_size));

			std::cout << "istream pos after: " << istream.tellg() << std::endl;

		}
		return (!istream.eof());
	}

	size_t get_file_size(std::string const& fpath)
	{
		std::ifstream getl(fpath, std::ifstream::ate | std::ifstream::binary);
		if (!getl)
			return (0);
		size_t file_length = getl.tellg();
		getl.close();
		return (file_length);
	}

} // namespace data

} // namespace webserv
