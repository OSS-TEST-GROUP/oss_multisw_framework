#ifndef ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_

#include <chrono>

#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "monitor/ContainerHealthMonitor.hpp"
#include "control/ROSServiceController.hpp"
#include "monitor/ROSServiceDiscovery.hpp"
#include "endpoint/EndPointFactory.hpp"

namespace ros2 {
namespace adsw_manager {

enum class AdswState{
  ADSW_INIT,
  ADSW_STARTED,
  ADSW_RUNNING,
  ADSW_PASUE,
  ADSW_TERMINATE
};

class ADSWManager {
  public:
    ADSWManager();
    virtual ~ADSWManager();

    static ADSWManager* Get();

    bool Initialize();

    bool Start();
    void Stop();
    void Process();

    std::shared_ptr<DDSTransmitter> GetTransmitter();
    std::shared_ptr<ServiceRegistry> GetServiceRegistry();
    std::shared_ptr<ServiceMonitor> GetServiceMonitor();
    std::shared_ptr<ROSServiceController> GetServiceController();
    std::shared_ptr<ROSServiceDiscovery> GetServiceDiscovery();

    const char* GetTopic(MessageType topic);

    void SetState(AdswState state);
    AdswState GetState();

    // UpdateServiceListResponse 수신 시 호출 — ADSW_RUNNING 전환
    virtual void ConfirmUpdateServiceList();

  private:
    static std::shared_ptr<ADSWManager> getShared();
    static void SetShared(std::shared_ptr<ADSWManager> instance);

  private:
    std::shared_ptr<DDSTransmitter> dds_transmitter_;
    std::shared_ptr<ServiceRegistry> service_registry_;
    std::shared_ptr<ServiceMonitor> service_monitor_;
    std::shared_ptr<ContainerHealthMonitor> container_health_monitor_;
    std::shared_ptr<ROSServiceController> service_controller_;
    std::shared_ptr<EndPointFactory> endpoint_factory_;

    AdswState adsw_state_;

    // UpdateServiceListRequest 재전송 제어
    static constexpr int32_t kUpdateListRetryMs  = 5000;   // ADSW_STARTED: 5초마다 재시도
    static constexpr int32_t kUpdateListResyncMs = 30000;  // ADSW_RUNNING: 30초마다 재동기화
    std::chrono::steady_clock::time_point last_update_list_sent_;
    bool update_list_sent_{false};

};

} // namespace adsw_manager
} // namespace ros2

#endif
