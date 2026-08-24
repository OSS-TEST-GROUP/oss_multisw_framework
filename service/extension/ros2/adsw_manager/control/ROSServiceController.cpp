#include "ROSServiceController.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "transmitter/MessageQueue.hpp"
#include "logger/Log.hpp"
#include "HelperUtils.hpp"
#include "service/TypeConvert.hpp"
#include "service/ServiceRegistry.hpp"
#include <sys/utsname.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <set>

namespace ros2 {
namespace adsw_manager {


ROSServiceController::ROSServiceController(){
    LOG_DEBUG("ROSServiceController create ");
    
    // Initialize environment detection variables
    is_container_env_ = false;
    is_ros2_env_ = false;
    is_native_arch_ = false;
    use_native_execution_ = false;
    execution_path_ = "Unknown";
}
ROSServiceController::~ROSServiceController(){

}

bool ROSServiceController::Initialize(){

    LOG_DEBUG("ROSServiceController::Initialize start ");    
    
    // Detect environment once during initialization
    is_container_env_ = DetectContainer();
    is_ros2_env_ = DetectROS2();
    is_native_arch_ = DetectNativeArchitecture();  // Detects x86_64, i386, i686, aarch64, arm64
    
    // Determine execution method based on priority: container → ROS2 → native
    use_native_execution_ = false;  // Default to container/ROS2 orchestration
    
    if (is_container_env_) {
        execution_path_ = "Container";
        use_native_execution_ = false;
    } else if (is_ros2_env_) {
        execution_path_ = "ROS2";
        use_native_execution_ = false;
    } else if (is_native_arch_) {
        execution_path_ = "Native";
        use_native_execution_ = true;
    } else {
        execution_path_ = "Unknown";
        LOG_ERROR("No supported environment detected during initialization");
    }
    
    LOG_DEBUG("Environment detection results:");
    LOG_DEBUG("  Container=" << (is_container_env_ ? "Yes" : "No"));
    LOG_DEBUG("  ROS2=" << (is_ros2_env_ ? "Yes" : "No"));
    LOG_DEBUG("  Native=" << (is_native_arch_ ? "Yes" : "No"));
    LOG_DEBUG("  Selected execution path: " << execution_path_);
    LOG_DEBUG("  Use native execution: " << (use_native_execution_ ? "Yes" : "No"));
    
    ros2node_controller_ = std::make_shared<ROS2NodeController>();

    container_controller_ = std::make_shared<ContainerController>();
    if (container_controller_ != nullptr){
        container_controller_->Initialize();
    }

    return true;

}
void ROSServiceController::Finalize(){
  
    if (container_controller_ != nullptr){
        container_controller_->Finalize();
    }
}

bool ROSServiceController::StartGroup(const std::string& group){
    if (!group.empty()){
        if (GetContainer()->ContainerSupported()) {
            return GetContainer()->StartContainer(group);
        }
    }
    return false;
}

bool ROSServiceController::StopGroup(const std::string& group){
    if (!group.empty()){
        if (GetContainer()->ContainerSupported()) {
            return GetContainer()->StopContainer(group);
        }
    }
    return false;
}


bool ROSServiceController::StartService(const std::string& service_name){

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if (registry != nullptr) {
        return StartService(registry->GetService(service_name));
    }
    return false;
}

bool ROSServiceController::StopService(const std::string& service_name){

    std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();

    if (registry != nullptr) {
        return StopService(registry->GetService(service_name));
    }
    return false;
}


bool ROSServiceController::StartService(std::shared_ptr<Service> service){
    
    if (service == nullptr) {
        LOG_ERROR("Received null service in StartService.");
        return false;
    }

    LOG_DEBUG("ROSServiceController::StartService start " << service->GetName());

    // Container-workload service: the container's own CMD (command_args) is the
    // ROS2 stack, and ProcessServiceStartRequest already ran StartContainer
    // (docker run w/ extra_args + command, or docker start) before reaching
    // here. There is nothing to `docker exec` — doing so would double-launch.
    // Reflect RUNNING and return; the health monitor promotes/demotes based on
    // the container's running state.
    if (service->IsContainerWorkload()) {
        LOG_DEBUG("ROSServiceController::StartService container-workload, container already started: "
                  << service->GetName());
        service->SetServiceState(ServiceState::RUNNING);
        return true;
    }

    std::string command;
    std::string launchFile = service->GetLaunchFile();

    // Use pre-detected environment from initialization
    LOG_DEBUG("Environment (pre-detected): Container=" << (is_container_env_ ? "Yes" : "No")
              << ", ROS2=" << (is_ros2_env_ ? "Yes" : "No")
              << ", Native=" << (is_native_arch_ ? "Yes" : "No"));
    LOG_DEBUG("Execution path: " << execution_path_ << ", Native=" << (use_native_execution_ ? "Yes" : "No"));

    if (use_native_execution_) {
        // Native execution (x86_64, aarch64) - use direct process spawning
        if (launchFile.empty()){
            // For direct execution, use FindExecutablePath to get full path
            std::string fullExePath = FindExecutablePath(service->GetPackage());
            // Simplified command that should reliably return PID
            command = "bash -c \"" + fullExePath + " >/dev/null 2>&1 & echo \\$!\"";
            LOG_DEBUG("Using executable path: " << fullExePath);
        }else{
            // For launch files, use ROS2 commands directly
            command = "bash -c \"source /opt/ros/humble/setup.bash 2>/dev/null || true && ros2 launch " + service->GetPackage() + " " + service->GetLaunchFile() + " >/dev/null 2>&1 & echo \\$!\"";
        }
    } else {
        // Use container/ROS2 orchestration - same code path for both
        LOG_DEBUG("Using container/ROS2 orchestration execution");
        std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
        std::string container_name = registry ? registry->GetGroupNameByService(service) : "";
        std::string basecommand = GetContainer()->BaseRos2Command(container_name);
        // Branch selection precedence (when basecommand is non-empty, i.e. docker exec):
        //   launchFile non-empty → ros2 launch
        //   IsDirectExec()       → bash -c "<package> & echo $!"   (binary or shebang script)
        //   else                 → ros2 run
        // Same precedence in the in-container self-execution path.
        if (basecommand.empty()) {
            // In-container self-execution
            if (!launchFile.empty()) {
                command = "bash -c \"source /opt/ros/humble/setup.bash 2>/dev/null || true && ros2 launch " + service->GetPackage() + " " + service->GetLaunchFile() + " >/dev/null 2>&1 & echo \\$!\"";
            } else if (service->IsDirectExec()) {
                // package field carries the binary/script path (absolute or in PATH).
                command = "bash -c \"" + service->GetPackage() + " >/dev/null 2>&1 & echo \\$!\"";
            } else {
                command = "bash -c \"source /opt/ros/humble/setup.bash 2>/dev/null || true && ros2 run " + service->GetPackage() + " " + service->GetName() + " >/dev/null 2>&1 & echo \\$!\"";
            }
        } else {
            // docker exec path: basecommand already = "docker exec <name> bash -c \"source ... && "
            // Just append the command and close the quote.
            if (!launchFile.empty()) {
                command = basecommand + "ros2 launch " + service->GetPackage() + " " + service->GetLaunchFile() + " >/dev/null 2>&1 & echo \\$!\"";
            } else if (service->IsDirectExec()) {
                command = basecommand + service->GetPackage() + " >/dev/null 2>&1 & echo \\$!\"";
            } else {
                command = basecommand + "ros2 run " + service->GetPackage() + " " + service->GetName() + " >/dev/null 2>&1 & echo \\$!\"";
            }
        }
    }

    //int32_t result = system(command.c_str());

    // Update regex to handle both PID formats (direct $! and container output)
    std::regex re_pid(R"(\b(\d+)\b)");  // For native x86: match any digits (PID)
    std::regex re_container(R"(process started with pid \[(\d+)\])");  // For container output
    
    std::string exeresult = HelperUtils::ExecCommand(command, "");
    LOG_DEBUG("Command executed: " << command);
    LOG_DEBUG("Command output: '" << exeresult << "' (length: " << exeresult.length() << ")");
    
    // Process raw output directly - trim whitespace and check if it's a valid PID
    std::string trimmedResult = exeresult;
    // Remove leading/trailing whitespace including newlines
    trimmedResult.erase(trimmedResult.begin(), std::find_if(trimmedResult.begin(), trimmedResult.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    trimmedResult.erase(std::find_if(trimmedResult.rbegin(), trimmedResult.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmedResult.end());
    
    LOG_DEBUG("Trimmed output: '" << trimmedResult << "' (length: " << trimmedResult.length() << ")");
    
    /* parse exe command result - handle both PID formats */
    if (!trimmedResult.empty()){
        std::smatch match;

        // For native x86: try to parse as direct PID first
        if (use_native_execution_) {
            // For native execution, we expect just a PID number
            try {
                uint64_t pid = std::stoull(trimmedResult);
                if (pid > 0) {
                    service->SetServiceState(ServiceState::RUNNING);
                    service->SetPID(pid);
                    LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (native direct)");
                    return true;
                }
            } catch (const std::exception& e) {
                LOG_DEBUG("Failed to parse PID directly: " << e.what() << ", trying regex...");
            }
        }

        // Fallback: try regex patterns for both native and container formats
        if (std::regex_match(trimmedResult, match, re_pid)) {
            uint64_t pid = std::stoi(match[1].str());
            service->SetServiceState(ServiceState::RUNNING);
            service->SetPID(pid);
            LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (regex direct)");
            return true;
        }
        // Then try container format
        if (std::regex_search(trimmedResult, match, re_container)) {
            uint64_t pid = std::stoi(match[1].str());
            service->SetServiceState(ServiceState::RUNNING);
            service->SetPID(pid);
            LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (container)");
            return true;
        }

        // Also try to split by lines and check each line (fallback for multi-line output)
        std::vector<std::string> lines = HelperUtils::SplitLines(exeresult);
        LOG_DEBUG("Number of output lines after SplitLines: " << lines.size());
        for (size_t i = 0; i < lines.size(); i++) {
            LOG_DEBUG("Line " << i << ": '" << lines[i] << "'");
            
            if (std::regex_match(lines[i], match, re_pid)) {
                uint64_t pid = std::stoi(match[1].str());
                service->SetServiceState(ServiceState::RUNNING);
                service->SetPID(pid);
                LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (direct from line)");
                return true;
            }
            if (std::regex_search(lines[i], match, re_container)) {
                uint64_t pid = std::stoi(match[1].str());
                service->SetServiceState(ServiceState::RUNNING);
                service->SetPID(pid);
                LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (container from line)");
                return true;
            }
        }
    } else {
        // Empty output - this shouldn't happen for native execution
        LOG_ERROR("Empty command output for: " << command);
        
        // For native execution, try to find the process by name as fallback
        if (use_native_execution_) {
            LOG_DEBUG("Attempting to find process by name as fallback...");
            std::string findPidCmd = "pgrep -f " + service->GetPackage() + " | tail -1";
            std::string pidResult = HelperUtils::ExecCommand(findPidCmd, "");
            
            if (!pidResult.empty()) {
                pidResult.erase(std::remove_if(pidResult.begin(), pidResult.end(), ::isspace), pidResult.end());
                try {
                    uint64_t pid = std::stoull(pidResult);
                    if (pid > 0) {
                        service->SetServiceState(ServiceState::RUNNING);
                        service->SetPID(pid);
                        LOG_DEBUG("StartService for :: " << service->GetPackage() << ", pid = " << pid << " (found by name)");
                        return true;
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Failed to parse fallback PID: " << e.what());
                }
            }
        }
        
        service->SetServiceState(ServiceState::NOT_RUNNING);
        LOG_ERROR("StartService failed :: " << command);
        return false;
    }

    return false;
}

bool ROSServiceController::StopService(std::shared_ptr<Service> service){
    if (service == nullptr) {
        LOG_ERROR("Received null service in StopService.");
        return false;
    }

    // Container-workload service: stopping the workload means stopping its
    // container (docker stop && rm). Must precede the pid==0 guard below —
    // these services carry no tracked PID (pid stays 0), so that guard would
    // otherwise refuse to stop them.
    if (service->IsContainerWorkload()) {
        std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
        std::string container_name = registry ? registry->GetGroupNameByService(service) : "";
        LOG_DEBUG("ROSServiceController::StopService container-workload, stopping container: " << container_name);
        bool ok = !container_name.empty() && GetContainer()->StopContainer(container_name);
        if (ok) {
            service->SetServiceState(ServiceState::NOT_RUNNING);
            service->SetPID(0);
        }
        return ok;
    }

    uint64_t pid = service->GetPID();
    
    if (pid == 0){
        pid = GetPID(service); // Get the PID of the service if needed
    }

    if (pid == 0) {
        LOG_ERROR("Service " << service->GetName() << " is not running.");
        return false; // Service is not running
    }
    
    LOG_DEBUG("ROSServiceController::StopService start, pid = " << pid);

    // Use pre-detected environment from initialization
    LOG_DEBUG("StopService environment: " << execution_path_ << ", Native=" << (use_native_execution_ ? "Yes" : "No"));
    
    std::string command;
    int32_t result;

    if (use_native_execution_) {
        // Native architecture (x86_64, aarch64) - use native process killing
        LOG_DEBUG("StopService using native execution");
        bool killed = KillProcessAndChildren(pid);
        result = killed ? 0 : 1;
        command = "kill process tree for PID " + std::to_string(pid);
    } else {
        // Use container/ROS2 orchestration - same code path for both
        LOG_DEBUG("StopService using container/ROS2 orchestration execution");
        std::shared_ptr<ServiceRegistry> registry = ADSWManager::Get()->GetServiceRegistry();
        std::string container_name = registry ? registry->GetGroupNameByService(service) : "";
        if (container_name.empty()) {
            // In-container self-execution
            command = "kill " + std::to_string(pid);
            result = system(command.c_str());
        } else {
            // docker exec: kill bash wrapper + pkill by match token for child
            // processes.  ResolveProcessMatchToken picks the right token
            // (launchfile / name / package / explicit override) consistent
            // with the liveness-polling logic, so direct-binary services don't
            // get missed by pkill -f using the service name.
            //
            // The pkill pattern MUST end-anchor with `$`, mirroring
            // IsServiceAliveInContainer. Without it, `pkill -f test_service`
            // matches its own enclosing `bash -c "... pkill ... test_service ..."`
            // and the wrapper bash kills itself — docker exec exits 143
            // (SIGTERM), system() returns nonzero, ProcessServiceStopRequest
            // reports FAILED to lifecycle_mgr, and lfc_monitor surfaces a
            // bogus "Stop Service Failed" even though the service actually
            // died. The token is left unquoted (same as the liveness check)
            // so the shell sees `\$` and passes literal `$` to pkill as a
            // regex end-anchor.
            const std::string token =
                service->ResolveProcessMatchToken(use_native_execution_);
            command = "docker exec " + container_name
                    + " bash -c \"kill " + std::to_string(pid) + " 2>/dev/null || true; "
                    + "pkill -TERM -f " + token + "\\$ 2>/dev/null || true; "
                    + "sleep 1.0; pkill -KILL -f " + token + "\\$ 2>/dev/null || true\"";
            result = system(command.c_str());
        }
    } 

    if (result == 0){
        LOG_DEBUG("StopService success :: " << command);
        service->SetServiceState(ServiceState::NOT_RUNNING);
        service->SetPID(0); // Clear the PID
        return true;
    }
    LOG_ERROR("StopService error : " << command);

#if 0    
    std::string keyword = "process has finished cleanly";
    std::string exeresult = HelperUtils::ExecCommand(command, keyword);
    std::vector<std::string> lines = HelperUtils::SplitLines(exeresult);

    if (lines.size() > 0){

        for (const auto& line : lines) {
            if (line.find(keyword) != std::string::npos) {
                service->SetServiceState(ServiceState::NOT_RUNNING);
                LOG_DEBUG("StopService success  :: " << service->GetPackage() << ", pid = " << pid);
                return true;
            }
        }
    }
#endif    
    
    LOG_ERROR("StopService failed :: " << service->GetPackage() << ", pid = " << pid);
    return false;
}

bool ROSServiceController::GetNodeLists(const std::shared_ptr<Service>& service) {
    if (service == nullptr) {
        LOG_ERROR("Received null service in GetNodeLists.");
        return false;
    }

    // kubectl exec -n default ros2-sample-all -- 
    // bash -c "source /opt/ros/humble/setup.bash && source /ros2_ws/install/setup.bash &&
    // ros2 pkg executables sample_ros2_app

    // sample_ros2_app listener
    // sample_ros2_app talker

    std::string basecommand = GetContainer()->BaseRos2Command("");

    std::string command = basecommand + " ros2 pkg executables " + service->GetPackage();
    
    std::string exeresult = HelperUtils::ExecCommand(command, std::string("") , 10);

    std::vector<std::string> lines = HelperUtils::SplitLines(exeresult);

    for (auto& line : lines){
        LOG_DEBUG("GetNodeLists :: " << line);
    }

    return true;
}

uint64_t ROSServiceController::GetPID(const std::shared_ptr<Service>& service) {

    if (service == nullptr) {
        LOG_ERROR("Received null service in GetPID.");
        return 0;
    }

    std::string basecommand = GetContainer()->BaseCommand("");
    std::string command = basecommand + " ps aux";

    // Command to run inside the container
    try {
        std::string output = HelperUtils::ExecCommand(command, std::string(""));
        std::vector<std::string> lines = HelperUtils::SplitLines(output);

        if (lines.empty()) {
            LOG_ERROR("No matching process found for: " << command);
        } else {
            uint64_t retpid = 0;
            for (const auto& line : lines) {
                try {
                    retpid = std::stoull(line); // Convert string to uint64_t
                } catch (...) {
                    continue;
                }
            }
            LOG_DEBUG("Matching PIDs: " << retpid);
            return retpid;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("ContainerController::GetPIDs failed " << command << ": " << e.what());
        return 0;
    }

    return 0;
}

bool ROSServiceController::ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message){
    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStartRequest.");
        return false;
    }

    LOG_DEBUG("Processing service start request for " << message->service_list().size() << " services.");
    // 이 판(ros2)은 ROS2 만 맡는다. MW_NATIVE 는 general 판 소관이다.
    const MwType req_mw_start = TypeConvert::ToMwType(message->mw_type());
    if (req_mw_start == MwType::ROS2) {
        std::vector<std::string> service_list = message->service_list();
        bool result = false;

        // 그룹 시작/정지는 컨테이너 환경에서만 지원한다는 전제로 항상 개별 처리한다.
        //
        // 원래는 message->is_group() 을 읽은 뒤 두 줄 아래에서 무조건 false 로
        // 덮어써서, 요청 필드가 읽히자마자 버려지고 있었다
        // (clang-analyzer-deadcode.DeadStores). 동작은 그대로 두고 죽은 저장만
        // 없앤다 — 요청의 is_group 은 이 판에서 쓰지 않는다는 뜻이다.
        //
        // 결과적으로 아래 `if (is_group)` 그룹 분기는 도달하지 않는다.
        // 지우거나 실제 환경 판정을 붙이는 건 동작 변경이라 별도 이슈로 둔다.
        const bool is_group = false;

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
                    if (!svc) {
                        continue;
                    }
                    std::string container_name =
                        registry->GetGroupNameByService(svc);
                    if (container_name.empty()) {
                        continue;
                    }
                    if (handled_containers.contains(container_name)) {
                        continue;
                    }
                    handled_containers.insert(container_name);

                    if (container_controller_->IsContainerRunning(container_name)) {
                        LOG_DEBUG("ROSServiceController: container " << container_name
                                  << " already running, starting service only: " << svc_name);
                        continue;
                    }
                    // Container not running. For a container-workload service the
                    // service IS the container's CMD, so a service Start must NOT
                    // silently docker-run the container behind the operator's back
                    // — that resurrects a container the operator explicitly Stopped.
                    // Gate it: mark the container failed so the service is skipped
                    // and its state stays NOT_RUNNING. The operator brings the
                    // container up via ctrl/StartContainer; ContainerHealthMonitor
                    // then promotes the workload service to RUNNING on its own.
                    if (svc->IsContainerWorkload()) {
                        LOG_DEBUG("ROSServiceController: container-workload " << container_name
                                  << " not running — service Start gated (start the container first): "
                                  << svc_name);
                        failed_containers.insert(container_name);
                        continue;
                    }
                    // Non-container-workload (services docker-exec'd into a shared
                    // long-running container): bring the hosting container up so the
                    // exec can land, as before.
                    LOG_DEBUG("ROSServiceController: container " << container_name
                              << " not running, starting it for service " << svc_name);
                    if (!container_controller_->StartContainer(container_name)) {
                        LOG_ERROR("StartContainer failed for " << container_name
                                  << "; services hosted in it will be skipped");
                        failed_containers.insert(container_name);
                    }
                }
            }

            auto container_failed = [&](const std::string& svc_name) -> bool {
                if (failed_containers.empty() || !registry) {
                    return false;
                }
                auto svc = registry->GetService(svc_name);
                if (!svc) {
                    return false;
                }
                const std::string container_name =
                    registry->GetGroupNameByService(svc);
                return !container_name.empty() &&
                       failed_containers.contains(container_name);
            };

            // Idempotency guard: if a service is already RUNNING /
            // INITIALIZING **with a real PID**, a duplicate StartServiceRequest
            // must not spawn another `docker exec ros2 run` — that would
            // orphan the prior PID, accumulate zombies, and pulse the
            // monitor-side state RUNNING↔NOT_RUNNING as the per-PID poller
            // catches each transient. Count the already-running ones toward
            // the SUCCESS verdict so the ctrl/* response is still SUCCESS.
            //
            // INITIALIZING + PID==0 is the cold-start case: ContainerHealthMonitor
            // sets every service to INITIALIZING when the hosting container
            // first goes RUNNING, but no process has actually been spawned
            // yet. PID rehydration can't find one either. Treating this as
            // "already running" silently swallows the user's Start click —
            // do NOT skip in that case; let StartService below actually
            // launch the docker exec.
            auto already_running = [&](const std::string& svc_name) -> bool {
                if (!registry) {
                    return false;
                }
                auto svc = registry->GetService(svc_name);
                if (!svc) {
                    return false;
                }
                ServiceState s = svc->GetServiceState();
                if (s == ServiceState::RUNNING) {
                    return true;
                }
                if (s == ServiceState::INITIALIZING && svc->GetPID() != 0) {
                    return true;
                }
                return false;
            };

            // NOTE: a MANUAL+NOT_RUNNING guard once lived here, but MANUAL
            // means "operator drives start/stop by hand" — that includes
            // lfc_monitor's Start button click. Refusing the click here
            // contradicts the policy's meaning. lifecycle_mgr's
            // PolicyManager already keeps automatic recovery from firing on
            // MANUAL (PolicyManual::Evaluate returns NONE for every state),
            // so the supervised pipeline never auto-restarts a MANUAL
            // service on its own. If a StartServiceRequest still arrives
            // here for a MANUAL service it's either operator intent or a
            // stray publisher writing directly to the raw topic — phantom
            // publishers are a separate problem (see
            // scripts/probe-stale-publisher.sh) and must be identified and
            // stopped at the source rather than masked here.

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
                    if (StartGroup(service)) {
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
                    if (StartService(service)) {
                        count++;
                    }
                }
            }
            result = count > 0;
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

        // 트랜스미터가 아직 없거나 이미 내려간 상태면 여기서 널 역참조로 죽는다.
    // 이 함수 안의 다른 모든 협력자(registry / monitor)는 널 검사를 하는데
    // 여기만 빠져 있었다. 응답을 못 보내는 건 실패로 보고한다.
    if (transmitter == nullptr) {
        LOG_ERROR("No transmitter — cannot send START_SERVICE_RESPONSE");
        return false;
    }

    return transmitter->SendMessage(MessageType::START_SERVICE_RESPONSE,
                                        reinterpret_cast<void*>(&start_service_response_));
    }
    // 응답하지 않는 것은 맞다. 다만 흔적 없이 사라지면 안 된다.
    LOG_DEBUG("StartServiceRequest ignored: mw_type " << static_cast<int>(req_mw_start)
              << " is not handled by this adsw_manager (ROS2)");
    return true;
}

bool ROSServiceController::ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message){
    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStopRequest.");
        return false;
    }

    LOG_DEBUG("Processing service stop request for " << message->service_list().size() << " services.");
    // 이 판(ros2)은 ROS2 만 맡는다. MW_NATIVE 는 general 판 소관이다.
    const MwType req_mw_stop = TypeConvert::ToMwType(message->mw_type());
    if (req_mw_stop == MwType::ROS2) {
        std::vector<std::string> service_list = message->service_list();
        bool result = false;

        // 그룹 시작/정지는 컨테이너 환경에서만 지원한다는 전제로 항상 개별 처리한다.
        //
        // 원래는 message->is_group() 을 읽은 뒤 두 줄 아래에서 무조건 false 로
        // 덮어써서, 요청 필드가 읽히자마자 버려지고 있었다
        // (clang-analyzer-deadcode.DeadStores). 동작은 그대로 두고 죽은 저장만
        // 없앤다 — 요청의 is_group 은 이 판에서 쓰지 않는다는 뜻이다.
        //
        // 결과적으로 아래 `if (is_group)` 그룹 분기는 도달하지 않는다.
        // 지우거나 실제 환경 판정을 붙이는 건 동작 변경이라 별도 이슈로 둔다.
        const bool is_group = false;

        if (service_list.empty()) {
            LOG_ERROR("No services provided to stop.");
            result = false;
        }
        else{
            LOG_DEBUG("Stopping services: " << (is_group ? "Group" : "Individual") << " - " << service_list.size() << " services.");

            int32_t count = 0;
            if (is_group) {
                for (const auto& service : service_list) {
                    if (StopGroup(service)) {
                        count++;
                    }
                }
            } else {
                for (const auto& service : service_list) {
                    if (StopService(service)) {
                        count++;
                    }
                }
            }
            result = count > 0;
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

        // 트랜스미터가 아직 없거나 이미 내려간 상태면 여기서 널 역참조로 죽는다.
    // 이 함수 안의 다른 모든 협력자(registry / monitor)는 널 검사를 하는데
    // 여기만 빠져 있었다. 응답을 못 보내는 건 실패로 보고한다.
    if (transmitter == nullptr) {
        LOG_ERROR("No transmitter — cannot send STOP_SERVICE_RESPONSE");
        return false;
    }

    return transmitter->SendMessage(MessageType::STOP_SERVICE_RESPONSE,
                                        reinterpret_cast<void*>(&stop_service_response_));
    }
    // 응답하지 않는 것은 맞다. 다만 흔적 없이 사라지면 안 된다.
    LOG_DEBUG("StopServiceRequest ignored: mw_type " << static_cast<int>(req_mw_stop)
              << " is not handled by this adsw_manager (ROS2)");
    return true;
}

// Environment detection methods (used only during initialization)
bool ROSServiceController::DetectNativeArchitecture() {
    struct utsname unameData;
    if (uname(&unameData) != 0) {
        return false;
    }
    
    std::string machine = unameData.machine;
    bool isNativeArch = (machine == "x86_64" || machine == "i386" || machine == "i686" || 
                         machine == "aarch64" || machine == "arm64");
    LOG_DEBUG("Architecture check: " << machine << " -> " << (isNativeArch ? "native (x86/ARM64)" : "not native"));
    return isNativeArch;
}

bool ROSServiceController::DetectROS2() {
    // Check if ROS2 is available
    bool hasROS2 = (access("/opt/ros/humble/setup.bash", F_OK) == 0);
    const char* rosDistro = getenv("ROS_DISTRO");
    
    LOG_DEBUG("ROS2 check: setup.bash=" << (hasROS2 ? "found" : "not found") 
              << ", ROS_DISTRO=" << (rosDistro ? rosDistro : "not set"));
    
    return hasROS2 || (rosDistro != nullptr);
}

bool ROSServiceController::DetectContainer() {
    // Check for container environment
    bool dockerEnv = (access("/.dockerenv", F_OK) == 0);
    bool k8sEnv = (access("/var/run/secrets/kubernetes.io", F_OK) == 0);
    
    // Check cgroup for container indicators
    bool cgroupContainer = false;
    std::ifstream cgroupFile("/proc/1/cgroup");
    if (cgroupFile.is_open()) {
        std::string line;
        while (std::getline(cgroupFile, line)) {
            // Traditional container patterns
            if (line.find("docker") != std::string::npos || 
                line.find("containerd") != std::string::npos ||
                line.find("kubepods") != std::string::npos ||
                line.find("/system.slice/docker") != std::string::npos) {
                cgroupContainer = true;
                break;
            }
            // Modern Docker cgroup v2 pattern: "0::/" or variations indicate container
            if (line.find("0::/") != std::string::npos) {
                cgroupContainer = true;
                break;
            }
        }
        cgroupFile.close();
    }
    
    bool inContainer = dockerEnv && k8sEnv ; // || cgroupContainer; ==> not reliable 
    LOG_DEBUG("Container check: docker=" << (dockerEnv ? "yes" : "no") 
              << ", k8s=" << (k8sEnv ? "yes" : "no") 
              << ", cgroup=" << (cgroupContainer ? "yes" : "no") 
              << " -> " << (inContainer ? "in container" : "not in container"));
    
    return inContainer;
}

bool ROSServiceController::KillProcessAndChildren(uint64_t pid) {
    if (pid == 0) {
        return false;
    }

    // First, try to kill child processes
    std::string kill_children_cmd = "pkill -TERM -P " + std::to_string(pid) + " 2>/dev/null || true";
    system(kill_children_cmd.c_str());
    
    // Then kill the main process
    std::string kill_main_cmd = "kill -TERM " + std::to_string(pid) + " 2>/dev/null";
    int result = system(kill_main_cmd.c_str());
    
    if (result != 0) {
        // If TERM failed, wait a bit and try KILL
        usleep(500000); // 0.5 seconds
        
        kill_children_cmd = "pkill -KILL -P " + std::to_string(pid) + " 2>/dev/null || true";
        system(kill_children_cmd.c_str());
        
        kill_main_cmd = "kill -KILL " + std::to_string(pid) + " 2>/dev/null";
        result = system(kill_main_cmd.c_str());
    }
    
    return (result == 0);
}

std::string ROSServiceController::FindExecutablePath(const std::string& packageName) {
    // Try to find the full path using 'which' command first
    std::string whichCmd = "which " + packageName + " 2>/dev/null";
    std::string whichResult = HelperUtils::ExecCommand(whichCmd, "");
    
    if (!whichResult.empty() && whichResult.find("not found") == std::string::npos) {
        // Remove trailing newline/whitespace
        whichResult.erase(std::remove_if(whichResult.begin(), whichResult.end(), ::isspace), whichResult.end());
        if (!whichResult.empty()) {
            LOG_DEBUG("Found executable using 'which': " << whichResult);
            return whichResult;
        }
    }
    
    // Fallback: check common installation paths
    std::vector<std::string> searchPaths = {
        "./" + packageName,  // Current running directory first
        "/oss/bin/" + packageName,
        "/opt/ros/humble/bin/" + packageName,
        "/usr/local/bin/" + packageName,
        "/usr/bin/" + packageName,
    };
    
    // Add user-specific ROS2 workspace paths
    const char* user = getenv("USER");
    if (user) {
        searchPaths.push_back("/home/" + std::string(user) + "/ros2_ws/install/" + packageName + "/bin/" + packageName);
        searchPaths.push_back("/home/" + std::string(user) + "/colcon_ws/install/" + packageName + "/bin/" + packageName);
    }
    
    // Add current directory build paths
    searchPaths.push_back("./build/install/" + packageName + "/bin/" + packageName);
    searchPaths.push_back("./install/" + packageName + "/bin/" + packageName);
    
    // Check each path for file existence
    for (const auto& path : searchPaths) {
        if (access(path.c_str(), F_OK) == 0) {  // File exists
            LOG_DEBUG("Found executable at: " << path);
            return path;
        }
    }
    
    // If still not found, log warning and return original package name
    LOG_ERROR("Could not find full path for executable '" << packageName << "', using package name directly (assuming it's in PATH)");
    return packageName;
}

} // namespace adsw_manager
} // namespace ros2
