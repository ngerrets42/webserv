#ifndef CONNECTION_H
# define CONNECTION_H

# include <stdexcept>
# include <cstring>

// networking
# include <unistd.h>
# include <string.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/errno.h>

void connection(int, struct sockaddr, socklen_t);

#endif // CONNECTION_H