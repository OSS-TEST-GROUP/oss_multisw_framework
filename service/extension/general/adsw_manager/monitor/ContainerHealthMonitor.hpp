#ifndef ROS2_ADSW_MANAGER_MONITOR_CONTAINER_HEALTH_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_MONITOR_CONTAINER_HEALTH_MONITOR_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>

namespace ros2 {
namespace adsw_manager {

class ContainerHealthMonitor {
  public:
    ContainerHealthMonitor();
    ~ContainerHealthMonitor();

    void Start();
    void Stop();

    // 초기 컨테이너 상태를 lifecycle_mgr에 보고 (직접 시작하지 않음).
    // lifecycle_mgr이 StartServiceRequest를 통해 컨테이너 재시작을 결정한다.
    void NotifyInitialContainerState();

  protected:
    // Overridable in tests to intercept state-change notifications.
    // This path does the full fan-out: per-service NotifyServiceState
    // through ServiceMonitor + dedicated NotifyContainerState DDS publish.
    // Only call on actual transitions to avoid retriggering recovery.
    virtual void NotifyContainerState(const std::string& container_name,
                                      bool is_running,
                                      int32_t exit_code);

    // F014 follow-up: publish-only path used by the periodic heartbeat.
    // dds_bridge forces VOLATILE durability so late-subscribing monitors
    // (e.g. lfc_monitor launched after adsw_manager) miss the one-shot
    // notify emitted on startup. Re-publishing the latest known state
    // every poll lets a fresh client catch up within one cycle, without
    // re-triggering ServiceMonitor's per-service fan-out (which would
    // poke lifecycle_mgr's recovery policy on every tick).
    virtual void PublishContainerStateHeartbeat(const std::string& container_name,
                                                 bool is_running,
                                                 int32_t exit_code);

  protected:
    std::map<std::string, bool> prev_state_;  // container_name -> was_running

    // Per-service liveness cache: service_name -> (last_seen_pid, was_alive).
    // Tracks the PID checked last cycle to detect the alive→dead transition
    // for individual ROS2 node processes inside a still-running container.
    std::map<std::string, std::pair<uint64_t, bool>> prev_service_;

  private:
    void PollLoop();
    void PollServicesInContainer(const std::string& container_name);

    std::thread       poll_thread_;
    std::atomic<bool> running_{false};

    static constexpr int kPollIntervalMs = 3000;
};

} // namespace adsw_manager
} // namespace ros2

#endif
