#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_MESSAGE_ENDPOINT_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_MESSAGE_ENDPOINT_HPP_

#include <gmock/gmock.h>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {

class MockDDSMessageEndpoint : public DDSMessageEndpoint {
  public:
    MockDDSMessageEndpoint() {
        type_support_ = std::make_shared<TypeSupport>(new LifecycleDiagReportPubSubType());
    }

    std::string GetTopic() {
        return "TestTopic";
    }

    std::string GetTypeName() {
        return "LifecycleDiagReport";
    }

    std::shared_ptr<TypeSupport> GetTypeSupport() {
        return type_support_;
    }

    void OnDataAvailable() {

    }

    std::shared_ptr<TypeSupport> type_support_;
};

}
}

#endif
