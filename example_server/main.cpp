#include <iostream>
#include "Server.h"

// This is so the file-descriptors and sockets actually get closed,
// which is important otherwise this machine will continuously believe
// the port used is still open.
void sigint_handler(int signum)
{
	std::cout << "Server shutting down..." << std::endl;
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	Server server;

	signal(SIGINT, sigint_handler);

	while (server.receive_connection()) ;

	return (EXIT_SUCCESS);
}