#include "Pollable.h"

namespace webserv {

void Pollable::notify(short revents, pollable_map_t& fd_map)
{
	switch (revents)
	{
		case POLLHUP:	this->on_pollhup(fd_map); return ;
		case POLLIN:	this->on_pollin(fd_map); return ;
		case POLLOUT:	this->on_pollout(fd_map); return ;
		default: return ;
	}
}

} // namespace webserv
