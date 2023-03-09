#ifndef PARSING_H
# define PARSING_H

# include "Core.h"
# include "../njson/njson.h"

#include "Server.h"
//#include "Socket.h"

namespace webserv {

std::vector<std::unique_ptr<Server>> parse_servers(njson::Json::pointer& root_node);
//std::vector<std::unique_ptr<Socket>> build_sockets(std::vector<std::unique_ptr<Server>>& servers);

} // namespace webserv

#endif // PARSING_H
