#ifndef SERVICE_CONTROLLER_HPP_
#define SERVICE_CONTROLLER_HPP_

#include "AppController.hpp"
#include "ContainerController.hpp"

#include <iostream>

namespace adsw_manager::agl {

    class ServiceController {

        public:
            ServiceController(AppController* app_controller, ContainerController* container_controller);
            virtual ~ServiceController() = default;

            virtual bool Initialize();
            virtual bool StartService(std::string appId, std::string contName);
            virtual bool StopService(std::string appId, std::string contName);

        private:
            AppController* app_controller_;
            ContainerController* container_controller_;

    };

} // namespace adsw_manager::agl

#endif
