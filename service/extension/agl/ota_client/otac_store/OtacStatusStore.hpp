#ifndef OTAC_STATUS_STORE_HPP_
#define OTAC_STATUS_STORE_HPP_

#include <iostream>
#include <map>

namespace ota_client::agl {
    class OtacStatusStore {

        public:
            bool Initialize();
            std::string GetStatus(std::string app_name);
            bool UpdateStatus(std::string app_name, std::string status);

        private:
            std::map<std::string, std::string> app_status;
    };
};

#endif