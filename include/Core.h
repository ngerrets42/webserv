#ifndef CORE_H
# define CORE_H

// Required C-STD libraries:
# include <sys/socket.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <fcntl.h>
# include <poll.h>
# include <string.h>

// Required C++-STD libraries:
# include <cstring>
# include <fstream>
# include <sstream>
# include <iostream>
# include <vector>
# include <unordered_map>
# include <bitset>

namespace webserv {

	typedef int sockfd_t;
	typedef struct sockaddr addr_t;
	typedef struct sockaddr_in addr_in_t;

} // webserv

#endif // CORE_H
