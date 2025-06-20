#include <iostream>
#include <thread>

#include "manager/LifecycleFactory.hpp"
#include "manager/LifecycleManager.hpp"
#include "logger/Log.hpp"

using namespace lifecycle_manager;

int main(int argc, char* argv[]) {

	LifecycleFactory* lifecycle = LifecycleFactory::GetInstance();

	std::shared_ptr<LifecycleManager> manager = lifecycle->GetManager();
	if (!manager->Initialize()) {
		LOG_ERROR("Failed to initialize the LifecycleManager");
	}

	if (!manager->Start()) {
		LOG_ERROR("Failed to start the LifecycleManager");
	}
	
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
}
