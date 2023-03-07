#include "Pollable.h"

namespace webserv {

void Pollable::notify(short revents, pollable_map_t& fd_map)
{
		if (revents & POLLHUP) {this->on_pollhup(fd_map); return ;}
		if (revents & POLLIN) {this->on_pollin(fd_map); return ;}
		if (revents & POLLOUT) {this->on_pollout(fd_map); return ;}
}

} // namespace webserv
