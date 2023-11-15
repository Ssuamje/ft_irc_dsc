#include <iostream>
#include "ServerKqueue.hpp"

int main(int ac, char* av[]) {
	if (ac != 3) {
		std::cerr << "Error : ./ircserv [port] [password]" << std::endl;
		return 1;
	}
	try {
		Server ircServ(av[1], av[2]);
		ircServ.init();
		ircServ.loop();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}
