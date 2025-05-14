#include <iostream>

#include "DiagnosticManager.hpp"

using namespace diagnostic_manager;

int main(int argc, char* argv[]) {
	std::cout << "Starting the DiagnosticManager." << std::endl;

	DiagnosticManager manager;
	if (!manager.Initialize()) {
		std::cerr << "Failed to initialize the DiagnosticManager." << std::endl;
	}

	if (!manager.Start()) {
		std::cerr << "Failed to start the DiagnosticManager." << std::endl;
	}

	return 0;
}
