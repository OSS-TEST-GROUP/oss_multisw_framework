
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_TYPE_SUPPORT_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_TYPE_SUPPORT_HPP_

#include <gmock/gmock.h>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    
// Mock classes for DDS dependencies
class MockTypeSupport : public TypeSupport {
public:
    std::string get_type_name()  { return "MockType"; }
    int register_type(DomainParticipant*)  { return RETCODE_OK; }
};


}
}

#endif