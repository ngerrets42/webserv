#include "Connection.h"
#include <iostream>

#define BUFFER_SIZE 8192 // usual max size of GET request is 8192
void connection(int fd, struct sockaddr address, socklen_t length)
{
	char buffer[BUFFER_SIZE] = {0};

	ssize_t ret = read(fd, buffer, BUFFER_SIZE);
	if (ret < 0)
		std::cout << "read() returned " << ret << std::endl;

	std::cout
		<< "=== received data: ===\n"
		<< buffer
		<< "\n=== End of data ==="
		<< std::endl;
	close(fd); // close connecton, normally would keep open (?)
}