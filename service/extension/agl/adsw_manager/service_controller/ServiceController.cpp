#include "ServiceController.hpp"

namespace adsw_manager::agl {

    ServiceController::ServiceController(AppController* app_controller, ContainerController* container_controller)
        : app_controller_(app_controller),
          container_controller_(container_controller) {}

    bool ServiceController::Initialize() {

        if(!app_controller_->Initialize()) {
            return false;
        }
        if(!container_controller_->Initialize()) {
            return false;
        }

        return true;

    }

    bool ServiceController::StartService(std::string appId, std::string contName) {

        if(container_controller_->IsContainerRunning(contName)) {
            // container on
            if(!app_controller_->IsAppRunning(appId, contName)) {
                // app off
                bool result = app_controller_->LaunchApp(appId, contName);
                return result;
            } else {
                // app on
                return true;
            }
        } else {
            // container off
            if(container_controller_->StartContainer(contName)) {
                // container on
                if(!app_controller_->IsAppRunning(appId, contName)) {
                    // app off
                    bool result = app_controller_->LaunchApp(appId, contName);
                    return result;
                }
            } else {
                // container off
                std::cout << "container start failed." << std::endl;
                return false;

            }
        }

        // 컨테이너가 필요없을 경우

        // if(!app_controller_.IsAppRunning(appId, contName)) {
        //     bool result = app_controller_.LaunchApp(appId, contName);
        //     return result;
        // } else {
        //     return true;
        // }

        return false;
    }

    bool ServiceController::StopService(std::string appId, std::string contName) {

        if(container_controller_->IsContainerRunning(contName)) {
            if(app_controller_->IsAppRunning(appId, contName)) {
                bool result = app_controller_->TerminateApp(appId, contName);
                if(result) {
                    bool c_result = container_controller_->StopContainer(contName);
                    return c_result;
                } else {
                    std::cout << " [[ " + contName + " ]] is not terminated." << std::endl;
                    return false;
                }
            } else {
                std::cout << "[[ " + appId + " ]] is already terminated." << std::endl;
                bool result = container_controller_->StopContainer(contName);
                if(result) {
                    std::cout << "Service is stopped successfully." << std::endl;
                    return true;
                } else {
                    std::cout << "Container [[ " + contName + " ]] is not stopped." << std::endl;
                    return false;
                }
            }
        } else {
            std::cout << "Container [[ " + contName + " ]] is already stopped." << std::endl;
            return true;
        }

        // 컨테이너가 필요없을 경우

        // if(app_controller_.IsAppRunning(appId, contName)) {
        //     bool result = app_controller_.TerminateApp(appId);
        //     return result;
        // } else {
        //     std::cout << "[[ " + appId + " ]] is already terminated." << std::endl;
        //     return true;
        // }

    }

}
