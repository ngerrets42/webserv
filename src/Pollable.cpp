#include "Pollable.h"
#include "Core.h"

namespace webserv {

void Pollable::notify(short revents, pollable_map_t& fd_map, sockfd_t fd)
{
	if (revents & POLLERR) {std::cout << "POLLERR on: " << this->get_fd() << std::endl; return; }
	if (revents & POLLHUP) {this->on_pollhup(fd_map, fd); return ;}
	if (revents & POLLIN) {this->on_pollin(fd_map); return ;}
	if (revents & POLLOUT) {this->on_pollout(fd_map); return ;}
	if (revents & POLLNVAL)
	{
		this->on_pollnval(fd_map);
		return;
	}
	std::cout << "Other event: " << revents << std::endl;
}

} // namespace webserv
