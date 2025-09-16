#ifndef MOCK_OTAC_STATUS_STORE_HPP_
#define MOCK_OTAC_STATUS_STORE_HPP_

#include <gmock/gmock.h>
#include "OtacStatusStore.hpp"
#include "ota_mgr/OtamClientServiceMsg.hpp"
#include <utility>  // std::declval

class MockOtacStatusStore : public ros2::ota_client::OtacStatusStore {
public:
    using StatusType = decltype(std::declval<OtaManager::NotifyAppUpdateStatus>().status());

    MOCK_METHOD(std::vector<std::string>, GetAppNameList, (), (const));
    MOCK_METHOD(bool, UpdateStatus, (const std::string, const std::string));
    virtual ~MockOtacStatusStore() = default;
};

#endif