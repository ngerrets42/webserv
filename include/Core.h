#ifndef CORE_H
# define CORE_H

// Required C-STD libraries:
# include <arpa/inet.h>
# include <fcntl.h>
# include <poll.h>
# include <string.h>
# include <sys/socket.h>
# include <unistd.h>

// Required C++-STD libraries:
# include <bitset>
# include <cstring>
# include <fstream>
# include <iostream>
# include <set>
# include <sstream>
# include <thread>
# include <unordered_map>
# include <vector>

namespace webserv {

	#define MAX_SEND_BUFFER_SIZE 8192

	typedef int sockfd_t;
	typedef struct sockaddr addr_t;
	typedef struct sockaddr_in addr_in_t;

} // namespace webserv

#endif // CORE_H
