#ifndef POLLABLE_H
# define POLLABLE_H

# include "Core.h"

namespace webserv {

class Pollable;
typedef std::unordered_map<sockfd_t, Pollable*> pollable_map_t;

class Pollable
{
	public:
	virtual ~Pollable() {}

	virtual sockfd_t get_fd(void) const = 0;

	void notify(short revents, pollable_map_t& fd_map);

	virtual short get_events(sockfd_t fd) const = 0;

	virtual bool should_destroy(void) const = 0;

	virtual void on_post_poll(pollable_map_t& fd_map) { (void)fd_map; };
	protected:
	virtual void on_pollin(pollable_map_t& fd_map) = 0;
	virtual void on_pollout(pollable_map_t& fd_map) = 0;
	virtual void on_pollhup(pollable_map_t& fd_map) = 0;
	virtual void on_pollnval(pollable_map_t& fd_map) { (void)fd_map; };
};


} // namespace webserv

#endif // POLLABLE_H
