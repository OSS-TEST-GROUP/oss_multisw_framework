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

    // Start all configured containers and notify lifecycle_mgr on completion.
    void AutoStartContainers();

  protected:
    // Overridable in tests to intercept state-change notifications.
    virtual void NotifyContainerState(const std::string& container_name,
                                      bool is_running,
                                      int32_t exit_code);

  protected:
    std::map<std::string, bool> prev_state_;  // container_name -> was_running

  private:
    void PollLoop();

    std::thread       poll_thread_;
    std::atomic<bool> running_{false};

    static constexpr int kPollIntervalMs = 3000;
};

} // namespace adsw_manager
} // namespace ros2

#endif
