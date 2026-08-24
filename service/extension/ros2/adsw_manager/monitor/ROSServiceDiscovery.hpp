#ifndef ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_ROS_SERVICE_DISCOVERY_HPP_

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include "ROS2Participant.hpp"
#include "ros2/ParticipantEntitiesInfo.hpp"


namespace ros2 {
namespace adsw_manager {

class ServiceMonitor;


class ROSServiceDiscovery {
  public:
    ROSServiceDiscovery(ServiceMonitor* service_monitor);
    virtual ~ROSServiceDiscovery() = default;
    bool Initialize();
    void Finalize();

    // virtual 인 이유:
    //   ROSDiscoveryInfoEndpointTest 는 ADSWManager::Get()->GetServiceDiscovery() 가
    //   MockROSServiceDiscovery 를 shared_ptr<ROSServiceDiscovery> 로 돌려주게 해
    //   두고 이 호출을 가로챈다. non-virtual 이면 정적 디스패치로 실제 구현이
    //   불려서 EXPECT_CALL 이 절대 만족되지 않는다.
    //   같은 파일의 링크 심(MockROSServiceDiscovery.hpp)은 이 타겟에서
    //   MOCK_ROS_SERVICE_DISCOVERY_IMPL 이 정의되지 않아 컴파일에서 빠지므로
    //   (실제 ROSServiceDiscovery.cpp 와의 중복 정의를 피하려는 의도적 가드다)
    //   가로챌 수단이 virtual 밖에 없다.
    virtual void ParseROSDiscoveryInfo(rmw_dds_common::msg::ParticipantEntitiesInfo& info);
    std::shared_ptr<ROS2Participant> GetROSParticipant(const std::string& participant_id);

  private:
    std::string guessService(const std::string& ns);

  private:
    ServiceMonitor* service_monitor_;
    std::map<std::string, std::shared_ptr<ROS2Participant>> ros_participants_;

    std::map<std::string, std::string> ros_group_participants_;
    bool ros_service_discovery_initialized_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
