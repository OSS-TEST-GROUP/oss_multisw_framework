#include "ServiceController.hpp"

namespace adsw_manager::agl {

    ServiceController::ServiceController(AppController* app_controller)
        : app_controller_(app_controller) {}

    bool ServiceController::Initialize() {

        if(!app_controller_->Initialize()) {
            return false;
        }

        return true;

    }

    bool ServiceController::StartService(std::string app_name) {

        if(!app_controller_->IsAppRunning(app_name)) {
            bool result = app_controller_->LaunchApp(app_name);
            return result;
        } else {
            std::cout << "[[ " << app_name << " ]] is already running." << std::endl;
            return true;
        }

    }

    bool ServiceController::StopService(std::string app_name) {

        if(app_controller_->IsAppRunning(app_name)) {
            bool result = app_controller_->TerminateApp(app_name);
            return result;
        } else {
            std::cout << "[[ " + app_name + " ]] is already terminated." << std::endl;
            return true;
        }

    }

}
