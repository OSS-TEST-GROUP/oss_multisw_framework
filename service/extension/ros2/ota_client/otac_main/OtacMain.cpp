#include "OtacMain.hpp"

#include <thread>

namespace ros2::ota_client {

    bool OtacMain::Initialize(const std::string& config_path) {

        status_store_ = std::make_shared<OtacStatusStore>();
        if (!status_store_->Initialize(config_path)) {
            return false;
        }

        manager_service_ = std::make_shared<OtacManagerService>(status_store_);

        if (!manager_service_->Initialize()) {
            return false;
        }

        return true;

    }

    void OtacMain::Start() {

        std::vector<std::string> app_list = status_store_->GetAppNameList();
        const std::string& client_name = status_store_->GetClientName();

        std::cout << "OtacMain::Start\n";
        if(manager_service_->Start()) {
            if (manager_service_->PublishClientRegistration(client_name, app_list)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                manager_service_->PublishClientRegistration(client_name, app_list);
            }

        }
    }

    void OtacMain::Stop() {

        manager_service_->Stop();

    }

}
