#include "ContainerHealthMonitor.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "ADSWManager.hpp"
#include "control/ROSServiceController.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/ServiceState.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

namespace {
// Check if a service's node process is alive inside the container.
//
// We can't trust the stored PID: StartService captures the PID echoed by
// `bash -c "... ros2 run pkg exe & echo $!"`, which is the bash subshell
// (PPID=1 after the outer bash exits), NOT the C++ node binary. The wrapper
// PID is reused fast by docker exec — `kill -0 <pid>` gives false positives.
//
// pgrep matched against the FULL command line catches the actual binary
// (cmdline = `/opt/ros/humble/lib/<pkg>/<svc>`) and the ros2-cli wrapper
// (cmdline = `python3 /opt/ros/humble/bin/ros2 run <pkg> <svc>`). The `$`
// anchor restricts matches to processes whose cmdline ENDS with the service
// name — that excludes our own check command (whose cmdline ends with quote
// + shell punctuation) and the bash wrapper (whose cmdline ends with `$!`).
//
// pkill -9 -f <svc> kills wrapper + ros2-cli + binary; once they're gone,
// pgrep returns zero matches → we report dead.
bool IsServiceAliveInContainer(const std::string& container,
                                const std::string& service_name) {
    std::string cmd = "docker exec " + container
                    + " pgrep -f " + service_name + "\\$"
                    + " >/dev/null 2>&1 && echo alive || echo dead";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return true;  // can't check — don't false-positive a death
    char buf[16] = {0};
    bool got = (fgets(buf, sizeof(buf), fp) != nullptr);
    pclose(fp);
    if (!got) return true;
    return std::string(buf).find("alive") != std::string::npos;
}
}  // namespace

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

            // Per-service PID polling: catches SIGKILL of an individual ROS2
            // node when the container itself is still running. docker-inspect
            // alone cannot detect this; we use `docker exec kill -0 <pid>`.
            if (is_running) {
                PollServicesInContainer(name);
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

void ContainerHealthMonitor::PollServicesInContainer(const std::string& container_name) {
    if (!ADSWManager::Get()) return;
    auto registry = ADSWManager::Get()->GetServiceRegistry();
    auto monitor  = ADSWManager::Get()->GetServiceMonitor();
    if (!registry || !monitor) return;

    auto services = registry->GetServicesByGroup(container_name);
    LOG_DEBUG("PollServicesInContainer container=" << container_name
              << " services=" << services.size());
    for (auto& svc : services) {
        // Poll any service with a known PID. State alone is unreliable:
        // ROSServiceDiscovery's NodeStarted callback sets state to INITIALIZING
        // via UpdateServiceState(CREATED) after StartService set it to RUNNING,
        // so most of the time a healthy service sits at INITIALIZING. We treat
        // a non-zero PID and a non-NOT_RUNNING state as "alive enough to watch".
        ServiceState st = svc->GetServiceState();
        if (st == ServiceState::NOT_RUNNING) continue;
        uint64_t pid = svc->GetPID();
        if (pid == 0) continue;

        const std::string& sname = svc->GetName();
        auto it = prev_service_.find(sname);
        // Reset cache entry if the PID changed (service restarted).
        if (it != prev_service_.end() && it->second.first != pid) {
            prev_service_.erase(it);
            it = prev_service_.end();
        }

        bool alive = IsServiceAliveInContainer(container_name, sname);
        bool prev_alive = (it != prev_service_.end()) ? it->second.second : true;
        LOG_DEBUG("PollSvc " << sname << " pid=" << pid
                  << " state=" << static_cast<int>(st)
                  << " alive=" << (alive ? "yes" : "no")
                  << " prev_alive=" << (prev_alive ? "yes" : "no"));

        if (prev_alive && !alive) {
            LOG_DEBUG("ContainerHealthMonitor: service " << sname
                      << " (pid=" << pid << ") died inside container " << container_name);
            // -1 sentinel: process died but we can't recover its exit code via
            // `kill -0`. PolicyOnFailure treats any non-zero as failure, so this
            // is consistent with the existing convention.
            monitor->OnServiceProcessDied(sname, -1);
            // Clear the cache entry — next start gets a fresh PID and we
            // re-track from there.
            prev_service_.erase(sname);
        } else {
            prev_service_[sname] = std::make_pair(pid, alive);
        }
    }
}

} // namespace adsw_manager
} // namespace ros2
