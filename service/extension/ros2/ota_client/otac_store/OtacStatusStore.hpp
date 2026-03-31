#ifndef OTAC_STATUS_STORE_HPP_
#define OTAC_STATUS_STORE_HPP_

#include <iostream>
#include <map>
#include <vector>
#include <optional>
#include <string>

namespace ros2::ota_client {
    class OtacStatusStore {

        public:
            bool Initialize(const std::string& config_path = "");
            std::optional<std::string> GetStatus(const std::string& app_name) const;
            virtual std::vector<std::string> GetAppNameList() const;
            virtual bool UpdateStatus(const std::string app_name, const std::string status);
            const std::string& GetClientName() const;

        private:
            std::map<std::string, std::string> app_status;
            std::string client_name_{"ros2_ota_client"};

            void LoadDefaults();
    };
};

#endif
