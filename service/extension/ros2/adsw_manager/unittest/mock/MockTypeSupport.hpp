
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_TYPE_SUPPORT_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_TYPE_SUPPORT_HPP_

#include <gmock/gmock.h>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    
// Mock classes for DDS dependencies
class MockTypeSupport : public TypeSupport {
public:
    MockTypeSupport() : TypeSupport() {}
    ~MockTypeSupport() {}

    MOCK_METHOD(std::string, get_type_name, (), ());
    MOCK_METHOD(int, register_type, (DomainParticipant*), ());
};


}
}

#endif