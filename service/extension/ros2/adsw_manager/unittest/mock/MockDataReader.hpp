#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DATA_READER_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DATA_READER_HPP_

#include <gmock/gmock.h>
#include <fastdds/dds/subscriber/DataReader.hpp>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

class MockDataReader : public DataReader {
public:
    MockDataReader() : DataReader(nullptr) {}
    ~MockDataReader() {}

    MOCK_METHOD(ReturnCode_t, take_next_sample, (void* data, SampleInfo* info), ());
    // Add other methods as needed for your tests
};

} // namespace adsw_manager
} // namespace ros2

#endif
