#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_ADSW_MANAGER_HPP_

#include <gmock/gmock.h>

#include "ADSWManager.hpp"

#include "MockTypeSupport.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

    // Mock ADSWManager
    class MockADSWManager : public ADSWManager {
    public:
        MockADSWManager() : ADSWManager() {}
        virtual ~MockADSWManager() {}

        MOCK_METHOD(bool, Initialize, (), ());
        //MOCK_METHOD(void, Finalize, (), ());
        MOCK_METHOD(bool, Start, (), ());
        MOCK_METHOD(void, Stop, (), ());
        MOCK_METHOD(void, Process, (), ());

        MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
        MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetServiceRegistry, (), ());
        MOCK_METHOD(std::shared_ptr<ServiceMonitor>, GetServiceMonitor, (), ());
        MOCK_METHOD(std::shared_ptr<ROSServiceController>, GetServiceController, (), ());
        MOCK_METHOD(std::shared_ptr<ROSServiceDiscovery>, GetServiceDiscovery, (), ());
        MOCK_METHOD(const char*, GetTopic, (ros2::adsw_manager::MessageType topic), ());
        MOCK_METHOD(void, SetState, (ros2::adsw_manager::AdswState state), ());
        MOCK_METHOD(ros2::adsw_manager::AdswState, GetState, (), ());
        MOCK_METHOD(void, ConfirmUpdateServiceList, (), ());

        // Note: Get() is not mocked - it directly returns shared_instance_.get()
    
    public:

    };

    extern MockADSWManager* Mock_ADSWManager;

    // Declare the shared instance (defined in MockADSWManager.cpp)
    extern std::shared_ptr<ADSWManager> shared_instance_;

} // namespace adsw_manager
} // namespace ros2

#endif
