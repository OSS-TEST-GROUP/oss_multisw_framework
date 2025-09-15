#ifndef OTAC_MAIN_HPP_
#define OTAC_MAIN_HPP_

#include <iostream>
#include <memory>

#include "OtacManagerService.hpp"
#include "OtacStatusStore.hpp"

namespace ros2::ota_client {

    class OtacMain {
        public:
            bool Initialize();
            void Start();
            void Stop();

        private:
            std::shared_ptr<OtacManagerService> manager_service_;
            std::shared_ptr<OtacStatusStore> status_store_;
    };
};

#endif