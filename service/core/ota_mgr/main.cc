#include <iostream>
#include <thread>

#include "otam_main.h"

int main(int argc, char* argv[]) {
	std::cout << "Starting OtaManager" << std::endl;

	OtamMain manager;

    try {
		manager.Initialize();
		manager.Start();
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	} catch (const std::exception& e) {
		std::string error_msg = "Failed to publish manager ready notification: " + std::string(e.what());
	}
	manager.Stop();
	return 0;
}
