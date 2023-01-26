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
	std::unordered_map<std::string, Location> location_map; // map of Location path string - Location class
};

} // webserv

#endif // SERVER_H
