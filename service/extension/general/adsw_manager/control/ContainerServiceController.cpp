#include "ContainerServiceController.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "transmitter/MessageQueue.hpp"
#include "logger/Log.hpp"
#include "HelperUtils.hpp"
#include "service/TypeConvert.hpp"
#include "service/ServiceRegistry.hpp"
#include <algorithm>
#include <set>

namespace ros2 {
namespace adsw_manager {


ContainerServiceController::ContainerServiceController(){
    LOG_DEBUG("ContainerServiceController create ");
}
ContainerServiceController::~ContainerServiceController(){

}

bool ContainerServiceController::Initialize(){

    LOG_DEBUG("ContainerServiceController::Initialize start ");

    container_controller_ = std::make_shared<ContainerController>();
    if (container_controller_ != nullptr){
        container_controller_->Initialize();
    }

    return true;

}
void ContainerServiceController::Finalize(){

    if (container_controller_ != nullptr){
        container_controller_->Finalize();
    }
}

bool ContainerServiceController::StartGroup(const std::string& group){
    if (!group.empty()){
        if (GetContainer()->ContainerSupported() == true){
            return GetContainer()->StartContainer(group);
        }
    }
    return false;
}

bool ContainerServiceController::StopGroup(const std::string& group){
    if (!group.empty()){
        if (GetContainer()->ContainerSupported() == true){
            return GetContainer()->StopContainer(group);
        }
    }
    return false;
}


bool ContainerServiceController::StartService(const std::string& service_name){

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if (registry != nullptr) {
        return StartService(registry->GetService(service_name));
    }
    return false;
}

bool ContainerServiceController::StopService(const std::string& service_name){

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if (registry != nullptr) {
        return StopService(registry->GetService(service_name));
    }
    return false;
}


bool ContainerServiceController::StartService(std::shared_ptr<Service> service){

    if (service == nullptr) {
        LOG_ERROR("Received null service in StartService.");
        return false;
    }

    LOG_DEBUG("ContainerServiceController::StartService start " << service->GetName());

    // Container-workload service: the container's own CMD (command_args) is the
    // workload, and ProcessServiceStartRequest already ran StartContainer
    // (docker run w/ extra_args + command, or docker start) before reaching
    // here. There is nothing to `docker exec` — doing so would double-launch.
    // Reflect RUNNING and return; the health monitor promotes/demotes based on
    // the container's running state.
    if (service->IsContainerWorkload()) {
        LOG_DEBUG("ContainerServiceController::StartService container-workload, container already started: "
                  << service->GetName());
        service->SetServiceState(ServiceState::RUNNING);
        return true;
    }

    // The general adsw_manager only orchestrates container workloads. Native
    // process / `ros2 run` / `ros2 launch` execution is intentionally not
    // supported here (that lives in the ros2 variant).
    LOG_ERROR("ContainerServiceController::StartService non-container service not supported in general adsw_manager: "
              << service->GetName());
    return false;
}

bool ContainerServiceController::StopService(std::shared_ptr<Service> service){
    if (service == nullptr) {
        LOG_ERROR("Received null service in StopService.");
        return false;
    }

    // Container-workload service: stopping the workload means stopping its
    // container (docker stop && rm).
    if (service->IsContainerWorkload()) {
        std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
        std::string container_name = registry ? registry->GetGroupNameByService(service) : "";
        LOG_DEBUG("ContainerServiceController::StopService container-workload, stopping container: " << container_name);
        bool ok = !container_name.empty() && GetContainer()->StopContainer(container_name);
        if (ok) {
            service->SetServiceState(ServiceState::NOT_RUNNING);
            service->SetPID(0);
        }
        return ok;
    }

    // The general adsw_manager only orchestrates container workloads. Native
    // process / docker-exec pkill teardown is intentionally not supported here.
    LOG_ERROR("ContainerServiceController::StopService non-container service not supported in general adsw_manager: "
              << service->GetName());
    return false;
}


bool ContainerServiceController::ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message){
    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStartRequest.");
        return false;
    }

    LOG_DEBUG("Processing service start request for " << message->service_list().size() << " services.");

    std::vector<std::string> service_list = message->service_list();
    bool is_group = message->is_group();
    bool result = false;

    // Check if running on x86_64 (native) - groups only supported in container environments
    is_group = false;

    if (service_list.empty()) {
        LOG_ERROR("No services provided to start.");
        result = false;
    }
    else{
        int32_t count = 0;
        LOG_DEBUG("Starting services: " << (is_group ? "Group" : "Individual") << " - " << service_list.size() << " services.");

        // Container ownership semantics — services live inside
        // their container.  Per-container we attempt at most one
        // start: if the container goes RUNNING (already, or after
        // our explicit start), services hosted in it proceed.  If
        // StartContainer fails, every service mapped to that
        // container is skipped and the overall result reflects that
        // — instead of silently trying StartService against a
        // non-existent container, which the docker-exec inside
        // StartService would then fail at, producing two layers of
        // confusing error logs and a misleading ctrl/* outcome.
        std::shared_ptr<ServiceRegistry> registry =
            ADSWManager::Get()->GetServiceRegistry();
        std::set<std::string> failed_containers;
        if (container_controller_ && registry) {
            std::set<std::string> handled_containers;
            for (const auto& svc_name : service_list) {
                std::shared_ptr<Service> svc = registry->GetService(svc_name);
                if (!svc) continue;
                std::string container_name =
                    registry->GetGroupNameByService(svc);
                if (container_name.empty()) continue;
                if (handled_containers.count(container_name)) continue;
                handled_containers.insert(container_name);

                if (container_controller_->IsContainerRunning(container_name)) {
                    LOG_DEBUG("ContainerServiceController: container " << container_name
                              << " already running, starting service only: " << svc_name);
                    continue;
                }
                LOG_DEBUG("ContainerServiceController: container " << container_name
                          << " not running, starting it for service " << svc_name);
                if (!container_controller_->StartContainer(container_name)) {
                    LOG_ERROR("StartContainer failed for " << container_name
                              << "; services hosted in it will be skipped");
                    failed_containers.insert(container_name);
                }
            }
        }

        auto container_failed = [&](const std::string& svc_name) -> bool {
            if (failed_containers.empty() || !registry) return false;
            auto svc = registry->GetService(svc_name);
            if (!svc) return false;
            const std::string container_name =
                registry->GetGroupNameByService(svc);
            return !container_name.empty() &&
                   failed_containers.count(container_name) > 0;
        };

        // Idempotency guard: if a service is already RUNNING /
        // INITIALIZING **with a real PID**, a duplicate StartServiceRequest
        // must not spawn another workload — that would orphan the prior PID,
        // accumulate zombies, and pulse the monitor-side state
        // RUNNING↔NOT_RUNNING as the per-PID poller catches each transient.
        // Count the already-running ones toward the SUCCESS verdict so the
        // ctrl/* response is still SUCCESS.
        //
        // INITIALIZING + PID==0 is the cold-start case: ContainerHealthMonitor
        // sets every service to INITIALIZING when the hosting container
        // first goes RUNNING, but no process has actually been spawned
        // yet. PID rehydration can't find one either. Treating this as
        // "already running" silently swallows the user's Start click —
        // do NOT skip in that case; let StartService below actually
        // launch it.
        auto already_running = [&](const std::string& svc_name) -> bool {
            if (!registry) return false;
            auto svc = registry->GetService(svc_name);
            if (!svc) return false;
            ServiceState s = svc->GetServiceState();
            if (s == ServiceState::RUNNING) return true;
            if (s == ServiceState::INITIALIZING && svc->GetPID() != 0) return true;
            return false;
        };

        if (is_group) {
            for (const auto& service : service_list) {
                if (container_failed(service)) {
                    LOG_ERROR("Skip start of group " << service
                              << " — its container is down");
                    continue;
                }
                if (already_running(service)) {
                    LOG_DEBUG("Group " << service
                              << " already running — skipping respawn");
                    count++;
                    continue;
                }
                if (StartGroup(service) == true) {
                    count++;
                }
            }
        } else {
            for (const auto& service : service_list) {
                if (container_failed(service)) {
                    LOG_ERROR("Skip start of " << service
                              << " — its container is down");
                    continue;
                }
                if (already_running(service)) {
                    LOG_DEBUG("Service " << service
                              << " already running — skipping respawn");
                    count++;
                    continue;
                }
                if (StartService(service) == true){
                    count++;
                }
            }
        }
        if (count > 0){
            result = true;
        }else{
            result = false;
        }
    }

    /* send message via message queue.. do not use local variable --> calss variable */
    if (!result) {
        start_service_response_.ret_code() = lifecyclemgr::ReturnCode::FAILED;
        LOG_ERROR("Failed to start service(s).");
    } else {
        start_service_response_.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
        LOG_DEBUG("Service(s) started successfully.");
    }

    // Broadcast a NotifyServiceState immediately so lfc_monitor's UI
    // flips RUNNING the moment the operator's ctrl/Start completes,
    // instead of waiting up to the LifecycleDiagReport heartbeat
    // (~30 s) for the change to surface. We don't gate on `result`
    // because even a partial / failed start may have mutated some
    // service entries (e.g. one of several services in the batch
    // launched fine), and the monitor wants the actual current snapshot.
    if (auto monitor = ADSWManager::Get()->GetServiceMonitor()) {
        monitor->NotifyServiceState();
    }

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();

    return transmitter->SendMessage(MessageType::START_SERVICE_RESPONSE,
                                    reinterpret_cast<void*>(&start_service_response_));
}

bool ContainerServiceController::ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message){
    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStopRequest.");
        return false;
    }

    LOG_DEBUG("Processing service stop request for " << message->service_list().size() << " services.");

    std::vector<std::string> service_list = message->service_list();
    bool is_group = message->is_group();
    bool result = false;

    // Check if running on x86_64 (native) - groups only supported in container environments
    is_group = false;

    if (service_list.empty()) {
        LOG_ERROR("No services provided to stop.");
        result = false;
    }
    else{
        LOG_DEBUG("Stopping services: " << (is_group ? "Group" : "Individual") << " - " << service_list.size() << " services.");

        int32_t count = 0;
        if (is_group) {
            for (const auto& service : service_list) {
                if (StopGroup(service) == true) {
                    count++;
                }
            }
        } else {
            for (const auto& service : service_list) {
                if (StopService(service) == true) {
                    count++;
                }
            }
        }
        if (count > 0) {
            result = true;
        } else {
            result = false;
        }
    }

    /* send message via message queue.. do not use local variable --> calss variable */
    if (!result) {
        stop_service_response_.ret_code() = lifecyclemgr::ReturnCode::FAILED;
        LOG_ERROR("Failed to stop service(s).");
    } else {
        stop_service_response_.ret_code() = lifecyclemgr::ReturnCode::SUCCESS;
        LOG_DEBUG("Service(s) stop successfully.");
    }

    // Same NotifyServiceState push as the start path — see commentary
    // in ProcessServiceStartRequest. lfc_monitor needs an immediate
    // state-change event for the badge to flip without waiting on the
    // periodic LifecycleDiagReport.
    if (auto monitor = ADSWManager::Get()->GetServiceMonitor()) {
        monitor->NotifyServiceState();
    }

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();

    return transmitter->SendMessage(MessageType::STOP_SERVICE_RESPONSE,
                                    reinterpret_cast<void*>(&stop_service_response_));
}

} // namespace adsw_manager
} // namespace ros2
