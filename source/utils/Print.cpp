#include "../../include/utils/Print.hpp"

Print::Print() {}

Print::~Print() {}

void Print::PrintLineNoColor(std::string message) {
	std::cout << message << std::endl;
}

void Print::PrintLineWithColor(std::string message, Color color) {
	std::cout << "\x1b[" << color << "m" << message << "\x1b[" << RESET << "m" << std::endl;
}

void Print::PrintMultiLineWithColor(std::string* message, int size, Color color) {
	for (int i = 0; i < size; i++) {
		std::cout << "\x1b[" << color << "m" << message[i] << "\x1b[" << RESET << "m" << std::endl;
	}
}

void Print::PrintMultiLineNoColor(std::string* message, int size) {
	for (int i = 0; i < size; i++) {
		std::cout << message[i] << std::endl;
	}
}
