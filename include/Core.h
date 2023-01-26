#ifndef CORE_H
# define CORE_H

// Required C-STD libraries:
# include <sys/socket.h>
# include <unistd.h>

// Required C++-STD libraries:
# include <fstream>
# include <sstream>
# include <iostream>
# include <vector>
# include <unordered_map>

namespace webserv {

	typedef int sockfd_t;
	typedef struct sockaddr sockaddr_t;

} // webserv

#endif // CORE_H
