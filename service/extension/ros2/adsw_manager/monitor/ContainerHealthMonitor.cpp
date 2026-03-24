#include "ContainerHealthMonitor.hpp"

#include <chrono>

#include "ADSWManager.hpp"
#include "control/ROSServiceController.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

ContainerHealthMonitor::ContainerHealthMonitor() {}

ContainerHealthMonitor::~ContainerHealthMonitor() {
    Stop();
}

void ContainerHealthMonitor::Start() {
    running_ = true;
    poll_thread_ = std::thread(&ContainerHealthMonitor::PollLoop, this);
    LOG_DEBUG("ContainerHealthMonitor started");
}

void ContainerHealthMonitor::Stop() {
    running_ = false;
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

void ContainerHealthMonitor::NotifyInitialContainerState() {
    // 컨테이너를 직접 시작하지 않고 현재 상태를 lifecycle_mgr에 보고한다.
    // lifecycle_mgr의 recovery policy가 StartServiceRequest를 통해 재시작을 결정한다.
    if (!ADSWManager::Get()) return;
    auto controller = ADSWManager::Get()->GetServiceController();
    if (!controller) return;

    auto container_ctrl = controller->GetContainer();
    if (!container_ctrl) return;

    auto containers = container_ctrl->GetAllContainers();
    for (const auto& container : containers) {
        const std::string& name = container->GetName();
        bool is_running = container_ctrl->IsContainerRunning(name);
        int32_t exit_code = is_running ? 0 : container_ctrl->GetContainerExitCode(name);

        LOG_DEBUG("ContainerHealthMonitor::NotifyInitialContainerState "
                  << name << " running=" << (is_running ? "true" : "false")
                  << " exit_code=" << exit_code);

        prev_state_[name] = is_running;
        NotifyContainerState(name, is_running, exit_code);
    }
}

void ContainerHealthMonitor::PollLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));

        if (!running_) break;

        if (!ADSWManager::Get()) continue;
        auto controller = ADSWManager::Get()->GetServiceController();
        if (!controller) continue;

        auto container_ctrl = controller->GetContainer();
        if (!container_ctrl) continue;

        auto containers = container_ctrl->GetAllContainers();
        for (const auto& container : containers) {
            const std::string& name = container->GetName();
            bool is_running = container_ctrl->IsContainerRunning(name);

            auto it = prev_state_.find(name);
            bool prev_running = (it != prev_state_.end()) ? it->second : false;

            if (is_running != prev_running) {
                int32_t exit_code = is_running ? 0 : container_ctrl->GetContainerExitCode(name);
                LOG_DEBUG("ContainerHealthMonitor: " << name
                          << " state changed: " << (prev_running ? "running" : "stopped")
                          << " -> " << (is_running ? "running" : "stopped")
                          << " exit_code=" << exit_code);
                NotifyContainerState(name, is_running, exit_code);
                prev_state_[name] = is_running;
            }
        }
    }

    LOG_DEBUG("ContainerHealthMonitor poll loop exited");
}

// virtual — overridable in tests
void ContainerHealthMonitor::NotifyContainerState(const std::string& container_name,
                                                   bool is_running,
                                                   int32_t exit_code) {
    if (!ADSWManager::Get()) return;
    auto monitor = ADSWManager::Get()->GetServiceMonitor();
    if (monitor) {
        monitor->OnContainerStateChanged(container_name, is_running, exit_code);
    }
}

} // namespace adsw_manager
} // namespace ros2
