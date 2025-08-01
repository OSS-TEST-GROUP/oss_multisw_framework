
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_HPP_

#include <gmock/gmock.h>

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {


class MockTopic {};
class MockDataReader {
public:
    uint64_t get_unread_count() { return 42; }
};
class MockDataWriter {
public:
    bool write(void*) { return true; }
};

class MockParticipant {
public:
    MockTopic* create_topic(const std::string&, const std::string&, int) { return new MockTopic(); }
};
class MockPublisher {
public:
    void get_default_datawriter_qos(int&) {}
    MockDataWriter* create_datawriter(MockTopic*, int&, DDSMessageEndpoint*, int) { return new MockDataWriter(); }
    void delete_datawriter(MockDataWriter* writer) { delete writer; }
};
class MockSubscriber {
public:
    void get_default_datareader_qos(int&) {}
    MockDataReader* create_datareader(MockTopic*, int&, DDSMessageEndpoint*, int) { return new MockDataReader(); }
    void delete_datareader(MockDataReader* reader) { delete reader; }
};


}
}

#endif
