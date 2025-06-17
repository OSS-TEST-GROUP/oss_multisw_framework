#include <iostream>
#include <thread>

#include "manager/LifecycleFactory.hpp"
#include "manager/LifecycleManager.hpp"

using namespace lifecycle_manager;

int main(int argc, char* argv[]) {
	std::cout << "Starting the LifecycleManager." << std::endl;

	LifecycleFactory* lifecycle = LifecycleFactory::GetInstance();

	std::shared_ptr<LifecycleManager> manager = lifecycle->GetManager();
	if (!manager->Initialize()) {
		std::cerr << "Failed to initialize the LifecycleManager." << std::endl;
	}

	if (!manager->Start()) {
		std::cerr << "Failed to start the LifecycleManager." << std::endl;
	}
	
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
}
