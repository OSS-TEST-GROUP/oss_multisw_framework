#ifndef ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <set>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

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
    virtual ~ADSWManager() = default;
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

    // Phantom-publisher guard. adsw_manager learns the GuidPrefix of each
    // trusted lifecycle_mgr when it receives UpdateServiceListResponse —
    // only lifecycle_mgr writes that topic, so any sender of that response
    // is a legitimate lifecycle_mgr. adsw_manager drops any
    // Start/StopServiceRequest arriving from a participant that has not
    // completed the UpdateServiceListResponse handshake.
    //
    // A set of prefixes (instead of a single "first wins" prefix) is
    // required because multiple lifecycle_mgrs — e.g. the production instance
    // AND a test-forked instance — can legitimately coexist on the same DDS
    // domain. Both must be allowed to send Start/Stop requests.
    void SetKnownLifecyclePublisher(
        const eprosima::fastdds::dds::InstanceHandle_t& handle);
    bool IsKnownLifecyclePublisher(
        const eprosima::fastdds::dds::InstanceHandle_t& handle);
    bool KnownLifecyclePublisherSet();

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

    AdswState adsw_state_ = AdswState::ADSW_INIT;   // GetState() 가 미초기화 값을 반환했다

    // UpdateServiceListRequest 재전송 제어
    static constexpr int32_t kUpdateListRetryMs  = 3000;   // ADSW_STARTED: 3초마다 재시도
    static constexpr int32_t kUpdateListResyncMs = 10000;  // ADSW_RUNNING: 10초마다 재동기화
    std::chrono::steady_clock::time_point last_update_list_sent_;
    bool update_list_sent_{false};

    // Set of GuidPrefixes (first 12 bytes of an InstanceHandle_t) of
    // participants we trust as lifecycle_mgr instances. A new prefix is
    // inserted every time a new UpdateServiceListResponse sender is seen.
    // Zombie publishers that never sent UpdateServiceListResponse remain blocked.
    std::mutex                                    lifecycle_prefix_mutex_;
    std::set<std::array<uint8_t, 12>>             lifecycle_trusted_prefixes_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
