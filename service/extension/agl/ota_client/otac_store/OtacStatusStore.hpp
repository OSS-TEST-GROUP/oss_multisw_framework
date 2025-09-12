#ifndef OTAC_STATUS_STORE_HPP_
#define OTAC_STATUS_STORE_HPP_

#include <iostream>
#include <map>
#include <vector>
#include <optional>

namespace ota_client::agl {
    class OtacStatusStore {

        public:
            bool Initialize();
            std::optional<std::string> GetStatus(const std::string& app_name) const;
            virtual std::vector<std::string> GetAppNameList() const;
            virtual bool UpdateStatus(const std::string app_name, const std::string status);

        private:
            std::map<std::string, std::string> app_status;
    };
};

#endif