#include "ContainerHealthMonitor.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "ADSWManager.hpp"
#include "control/ContainerServiceController.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/ServiceState.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "logger/Log.hpp"

// F013: NotifyContainerState DDS publish
#include "lifecycle/NotifyContainerState.hpp"
#include "endpoint/MessageTypes.hpp"

namespace general {
namespace adsw_manager {

namespace {
// Check if a service's process tree is alive inside the container.
//
// We can't trust the stored PID: StartService captures the PID echoed by
// `bash -c "... & echo $!"`, which is the bash subshell or ros2-cli wrapper,
// NOT the actual node/binary. The wrapper PID also gets reused fast by
// docker exec — `kill -0 <pid>` gives false positives.
//
// pgrep matched against the FULL command line (-f) catches the relevant
// process. The `$` end-anchor restricts matches to processes whose cmdline
// ENDS with the match token — that excludes the bash wrapper (cmdline ends
// with `$!`) and our own check shell (cmdline ends with quote/punctuation).
//
// `token` should come from Service::ResolveProcessMatchToken(), which picks
// launchfile / name / package / explicit-override appropriately.
bool IsServiceAliveInContainer(const std::string& container,
                                const std::string& token) {
    if (token.empty()) return true;  // no token → can't check; assume alive
    std::string cmd = "docker exec " + container
                    + " pgrep -f " + token + "\\$"
                    + " >/dev/null 2>&1 && echo alive || echo dead";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return true;  // can't check — don't false-positive a death
    char buf[16] = {0};
    bool got = (fgets(buf, sizeof(buf), fp) != nullptr);
    pclose(fp);
    if (!got) return true;
    return std::string(buf).find("alive") != std::string::npos;
}

// Look up the PID of a service inside the container using the same token
// scheme as IsServiceAliveInContainer. Returns 0 when nothing matches (so
// callers can use 0 to mean "not running / unknown"). The `$` end-anchor
// excludes the bash wrapper line ending in `$!` and our own check shell.
//
// Used for cold-start PID rehydration: when adsw_manager restarts while the
// service container kept running, ServiceRegistry comes up with pid_=0 for
// every service and our PID-based liveness path is dead-on-arrival. Probing
// pgrep recovers the live PID so the poll loop can resume.
uint64_t LookupServicePidInContainer(const std::string& container,
                                      const std::string& token) {
    if (token.empty()) return 0;
    std::string cmd = "docker exec " + container
                    + " pgrep -f " + token + "\\$"
                    + " 2>/dev/null | head -1";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return 0;
    char buf[32] = {0};
    bool got = (fgets(buf, sizeof(buf), fp) != nullptr);
    pclose(fp);
    if (!got) return 0;
    try {
        return std::stoull(buf);
    } catch (...) {
        return 0;
    }
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
            int32_t exit_code = is_running ? 0 : container_ctrl->GetContainerExitCode(name);

            auto it = prev_state_.find(name);
            bool prev_running = (it != prev_state_.end()) ? it->second : false;
            bool first_time = (it == prev_state_.end());

            if (first_time || is_running != prev_running) {
                LOG_DEBUG("ContainerHealthMonitor: " << name
                          << " state changed: " << (prev_running ? "running" : "stopped")
                          << " -> " << (is_running ? "running" : "stopped")
                          << " exit_code=" << exit_code);
                NotifyContainerState(name, is_running, exit_code);
                prev_state_[name] = is_running;
            } else {
                // F014 follow-up heartbeat: re-publish the latest known
                // state every cycle so monitors that connected after the
                // initial publish still see the container.  dds_bridge's
                // VOLATILE durability would otherwise leave them blank.
                PublishContainerStateHeartbeat(name, is_running, exit_code);
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

    // Existing path: fan out as per-service NotifyServiceState so recovery
    // policy keeps working.
    auto monitor = ADSWManager::Get()->GetServiceMonitor();
    if (monitor) {
        monitor->OnContainerStateChanged(container_name, is_running, exit_code);
    }

    // F013: also publish a dedicated NotifyContainerState DDS message so
    // lifecycle_mgr can update its containers_ map and the monitor sees
    // container-level state changes directly (independent of services).
    PublishContainerStateHeartbeat(container_name, is_running, exit_code);
}

// virtual — overridable in tests
void ContainerHealthMonitor::PublishContainerStateHeartbeat(
        const std::string& container_name,
        bool is_running,
        int32_t exit_code) {
    if (!ADSWManager::Get()) return;
    auto transmitter = ADSWManager::Get()->GetTransmitter();
    const char* topic = ADSWManager::Get()->GetTopic(MessageType::NOTIFY_CONTAINER_STATE);
    if (!transmitter || !topic) return;

    auto endpoint = transmitter->GetMessageEndpoint(topic);
    if (!endpoint) return;

    lifecyclemgr::NotifyContainerState payload;
    payload.container_list().push_back(container_name);
    payload.container_state_list().push_back(
        is_running ? lifecyclemgr::ContainerState::CONTAINER_RUNNING
                   : lifecyclemgr::ContainerState::CONTAINER_STOPPED);
    payload.exit_code_list().push_back(exit_code);
    endpoint->Write(&payload);
}

void ContainerHealthMonitor::PollServicesInContainer(const std::string& container_name) {
    if (!ADSWManager::Get()) return;
    auto controller = ADSWManager::Get()->GetServiceController();
    auto registry   = ADSWManager::Get()->GetServiceRegistry();
    auto monitor    = ADSWManager::Get()->GetServiceMonitor();
    if (!registry || !monitor) return;

    const bool is_native = controller && controller->IsNativeExecution();

    auto services = registry->GetServicesByGroup(container_name);
    LOG_DEBUG("PollServicesInContainer container=" << container_name
              << " services=" << services.size()
              << " native=" << (is_native ? "yes" : "no"));
    for (auto& svc : services) {
        // Poll every service in the container — including NOT_RUNNING ones.
        // The reason we don't skip NOT_RUNNING anymore: operators (or the
        // container's own entrypoint) can `docker exec` the service binary
        // directly, outside the StartService control path. lifecycle_mgr
        // should still notice and report RUNNING in that case. So if we
        // find a matching process for a NOT_RUNNING service, we promote it
        // through INITIALIZING → RUNNING the same way we do on cold-start.
        ServiceState st = svc->GetServiceState();

        // Container-workload service: its liveness IS the container's running
        // state, not an in-container process match. This loop is only reached
        // when the container is running (guarded by the caller), so the
        // workload is up — promote INITIALIZING → RUNNING and skip the
        // pgrep/PID rehydration machinery (the script's process name won't
        // match ResolveProcessMatchToken, which would strand it at
        // INITIALIZING). Death is handled by the container-state path
        // (OnContainerStateChanged → NOT_RUNNING).
        if (svc->IsContainerWorkload()) {
            if (st == ServiceState::INITIALIZING) {
                svc->SetServiceState(ServiceState::RUNNING);
                monitor->NotifyServiceState();
                LOG_DEBUG("PollSvc " << svc->GetName()
                          << " container-workload → RUNNING");
            }
            continue;
        }

        // Pick the right cmdline token for this service's invocation mode:
        // launchfile / name / package / explicit override.
        const std::string token = svc->ResolveProcessMatchToken(is_native);
        const std::string& sname = svc->GetName();

        uint64_t pid = svc->GetPID();
        if (pid == 0) {
            // PID rehydration. Two callers land here:
            //   1) Cold-start: adsw_manager restarted while the container
            //      kept running; registry comes back with pid_=0 even
            //      though the service is alive in the container.
            //   2) External start: operator ran `docker exec /path/to/svc`
            //      without going through StartService.
            // In both cases we look up the live PID via the same pgrep
            // token IsServiceAliveInContainer uses; if found, populate it.
            pid = LookupServicePidInContainer(container_name, token);
            if (pid == 0) {
                // No live process. NOT_RUNNING stays NOT_RUNNING (nothing
                // to do); INITIALIZING/RUNNING also wait for next cycle
                // since we have no PID to liveness-check.
                if (st != ServiceState::NOT_RUNNING) {
                    LOG_DEBUG("PollSvc " << sname
                              << " pid=0 rehydrate miss (token='" << token
                              << "') — leaving state="
                              << static_cast<int>(st));
                }
                continue;
            }
            svc->SetPID(pid);
            // Externally-started service: lift from NOT_RUNNING to
            // INITIALIZING so the promotion block below can elevate it to
            // RUNNING on this same iteration (alive check will pass).
            if (st == ServiceState::NOT_RUNNING) {
                svc->SetServiceState(ServiceState::INITIALIZING);
                st = ServiceState::INITIALIZING;
                monitor->NotifyServiceState();
                LOG_DEBUG("PollSvc " << sname
                          << " externally started — adopting pid=" << pid
                          << ", lifting NOT_RUNNING -> INITIALIZING");
            } else {
                LOG_DEBUG("PollSvc " << sname
                          << " PID rehydrated to " << pid
                          << " (token='" << token << "')");
            }
        }

        auto it = prev_service_.find(sname);
        // Reset cache entry if the PID changed (service restarted).
        if (it != prev_service_.end() && it->second.first != pid) {
            prev_service_.erase(it);
            it = prev_service_.end();
        }

        bool alive = IsServiceAliveInContainer(container_name, token);
        bool prev_alive = (it != prev_service_.end()) ? it->second.second : true;
        LOG_DEBUG("PollSvc " << sname << " pid=" << pid
                  << " token='" << token << "'"
                  << " state=" << static_cast<int>(st)
                  << " alive=" << (alive ? "yes" : "no")
                  << " prev_alive=" << (prev_alive ? "yes" : "no"));

        if (prev_alive && !alive) {
            LOG_DEBUG("ContainerHealthMonitor: service " << sname
                      << " (pid=" << pid << ", token='" << token << "') "
                      << "died inside container " << container_name);
            // -1 sentinel: process died but we can't recover its exit code via
            // `kill -0`. PolicyOnFailure treats any non-zero as failure, so this
            // is consistent with the existing convention.
            monitor->OnServiceProcessDied(sname, -1);
            // Clear the cache entry — next start gets a fresh PID and we
            // re-track from there.
            prev_service_.erase(sname);
        } else {
            // Promote INITIALIZING → RUNNING for services that are
            // confirmed alive by the in-container PID check. Without this,
            // non-ROS2 binaries (and ROS2 nodes whose discovery callback
            // arrives via UpdateServiceState(CREATED)) sit at INITIALIZING
            // forever and lifecycle_mgr/lfc_monitor never report RUNNING.
            // Using the per-PID liveness check is the same signal we
            // already trust for death detection, just applied in the
            // healthy direction.
            if (alive && st == ServiceState::INITIALIZING) {
                svc->SetServiceState(ServiceState::RUNNING);
                monitor->NotifyServiceState();
                LOG_DEBUG("ContainerHealthMonitor: service " << sname
                          << " promoted INITIALIZING -> RUNNING (pid=" << pid
                          << ", alive by pgrep)");
            }
            prev_service_[sname] = std::make_pair(pid, alive);
        }
    }
}

} // namespace adsw_manager
} // namespace general
