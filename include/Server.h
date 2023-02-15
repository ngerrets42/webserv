#ifndef SERVER_H
# define SERVER_H

# include "Core.h"

namespace webserv {

class Location
{
	// Location config settings
};

class Server
{
	// hold server config

	private:

	Location default_location;

	std::unordered_map<std::string, Location> location_map; // map of Location path string - Location class
};

} // namespace webserv

#endif // SERVER_H
