#ifndef HTML_H
# define HTML_H

# include "Core.h"

namespace webserv {

std::string build_index(std::string const& dir_path, std::string const& dir_name);

} // webserv

#endif // HTML_H
