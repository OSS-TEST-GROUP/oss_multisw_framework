#ifndef SERVICE_CONTROLLER_HPP_
#define SERVICE_CONTROLLER_HPP_

#include "AppController.hpp"

#include <iostream>

namespace adsw_manager::agl {

    class ServiceController {

        public:
            ServiceController(AppController* app_controller);
            virtual ~ServiceController() = default;

            virtual bool Initialize();
            virtual bool StartService(std::string app_name);
            virtual bool StopService(std::string app_name);

        private:
            AppController* app_controller_;

    };

} // namespace adsw_manager::agl

#endif
