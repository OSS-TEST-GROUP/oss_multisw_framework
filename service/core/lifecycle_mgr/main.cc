#include <iostream>
#include <thread>

#include "LifecycleManager.hpp"

using namespace lifecycle_manager;

int main(int argc, char* argv[]) {
	std::cout << "Starting the LifecycleManager." << std::endl;

	LifecycleManager manager;
	if (!manager.Initialize()) {
		std::cerr << "Failed to initialize the LifecycleManager." << std::endl;
	}

	if (!manager.Start()) {
		std::cerr << "Failed to start the LifecycleManager." << std::endl;
	}
	
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
}
