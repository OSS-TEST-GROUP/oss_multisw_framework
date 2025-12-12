
#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_ROS_SERVICE_DISCOVERY_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_ROS_SERVICE_DISCOVERY_HPP_

#include <gmock/gmock.h>
#include "monitor/ROSServiceDiscovery.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {
    


class MockROSServiceDiscovery : public ROSServiceDiscovery {
  public:
    MockROSServiceDiscovery(ServiceMonitor* service_monitor) : ROSServiceDiscovery(service_monitor) {}
    ~MockROSServiceDiscovery() override = default;

    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(void, ParseROSDiscoveryInfo, (rmw_dds_common::msg::ParticipantEntitiesInfo& info), ());
    MOCK_METHOD(std::shared_ptr<ROS2Participant>, GetROSParticipant, (const std::string& participant_id), ());

};    

MockROSServiceDiscovery* Mock_ROSServiceDiscovery;

// Only define mock implementations when MOCK_ROS_SERVICE_DISCOVERY_IMPL is defined
// This prevents multiple definition errors when linking with real ROSServiceDiscovery.cpp
#ifdef MOCK_ROS_SERVICE_DISCOVERY_IMPL

ROSServiceDiscovery::ROSServiceDiscovery(ServiceMonitor* service_monitor) {
}
ROSServiceDiscovery::~ROSServiceDiscovery() {
}
bool ROSServiceDiscovery::Initialize() {
    return Mock_ROSServiceDiscovery->Initialize();
}
void ROSServiceDiscovery::Finalize() {
    Mock_ROSServiceDiscovery->Finalize();
}
void ROSServiceDiscovery::ParseROSDiscoveryInfo(rmw_dds_common::msg::ParticipantEntitiesInfo& info) {
    Mock_ROSServiceDiscovery->ParseROSDiscoveryInfo(info);
}
std::shared_ptr<ROS2Participant> ROSServiceDiscovery::GetROSParticipant(const std::string& participant_id) {
    return Mock_ROSServiceDiscovery->GetROSParticipant(participant_id);
}

#endif // MOCK_ROS_SERVICE_DISCOVERY_IMPL

}
}

#endif