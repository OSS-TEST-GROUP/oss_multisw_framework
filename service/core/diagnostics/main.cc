#include <iostream>
#include <thread>

#include "DiagnosticManager.hpp"
#include "logger/Log.hpp"

using namespace diagnostic_manager;

int main(int argc, char* argv[]) {
	LOG_DEBUG("Starting the DiagnosticManager.");

	DiagnosticManager manager;
	if (!manager.Initialize()) {
		LOG_ERROR("Failed to initialize the DiagnosticManager.");
	}

	if (!manager.Start()) {
		LOG_ERROR("Failed to start the DiagnosticManager.");
	}

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
}
