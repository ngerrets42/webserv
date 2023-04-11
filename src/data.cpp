#include "data.h"

#include <sys/stat.h>

namespace webserv {

namespace data
{

	bool file_is_valid(std::string const& fpath)
	{
		struct stat buf;
		if(stat(fpath.c_str(), &buf) == 0)
		{
			if( (buf.st_mode & S_IFREG) != 0 )
				return (true);
		}
		return (false);
	}

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
		{
			std::cout << "No more file!" << std::endl;
			return (false); // No more file left to conquer
		}

		std::vector<char> buffer(buffer_size);
		istream.read( reinterpret_cast<char*>(buffer.data()) , buffer.size());
		// {
		// 	std::cerr << "Failed to read from file! (" << istream.tellg() << ')' << std::endl;
		// 	// return false;
		// }

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
			istream.seekg(istream.tellg() - static_cast<std::ifstream::pos_type>(buffer.size() - send_size));
		}
		// std::cout << "Send " << send_size << " bytes." << std::endl;
		return (!istream.eof());
	}

	size_t get_file_size(std::string const& fpath)
	{
		std::ifstream getl(fpath, std::ifstream::ate | std::ifstream::binary);
		std::ifstream::pos_type pos = getl.tellg();
		getl.close();
		getl.clear();
		return (pos);
	}

} // namespace data

} // namespace webserv
