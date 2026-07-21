#include <iostream>
#include <thread>

#include "ADSWManager.hpp"
#include "logger/Log.hpp"

int main(int argc, char* argv[]) {

	general::adsw_manager::ADSWManager* manager = general::adsw_manager::ADSWManager::Get();

	if (!manager->Initialize()) {
		LOG_ERROR("Failed to initialize the LifecycleManager");
	}

	if (!manager->Start()) {
		LOG_ERROR("Failed to start the LifecycleManager");
	}
	
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		manager->Process();

	}


	return 0;
}
