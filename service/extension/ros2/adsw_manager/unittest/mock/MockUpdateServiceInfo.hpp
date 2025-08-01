#ifndef MOCK_UPDATE_SERVICE_INFO_HPP_
#define MOCK_UPDATE_SERVICE_INFO_HPP_

#include "monitor/UpdateServiceInfo.hpp"
#include <gmock/gmock.h>

namespace ros2 {
namespace adsw_manager {

class MockUpdateServiceInfo : public UpdateServiceInfo {
public:
    MockUpdateServiceInfo(const std::string& name = "", const std::string& namespace_name = "", ServiceEvent event = ServiceEvent::CREATED, ServiceState state = ServiceState::NOT_RUNNING)
        : UpdateServiceInfo(name, namespace_name, event, state) {}
    // No virtual methods to mock, but you can set fields directly in tests
};

MockUpdateServiceInfo* Mock_UpdateServiceInfo;

UpdateServiceInfo::UpdateServiceInfo(const std::string& name, const std::string& namespace_name, ServiceEvent event, ServiceState state)
{

}





} // namespace adsw_manager
} // namespace ros2

#endif // MOCK_UPDATE_SERVICE_INFO_HPP_
