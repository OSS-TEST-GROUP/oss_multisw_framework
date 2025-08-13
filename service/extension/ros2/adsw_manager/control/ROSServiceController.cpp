#include "ROSServiceController.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "transmitter/MessageQueue.hpp"
#include "logger/Log.hpp"
#include "HelperUtils.hpp"

namespace ros2 {
namespace adsw_manager {


ROSServiceController::ROSServiceController(){
    LOG_DEBUG("ROSServiceController create ");
}
ROSServiceController::~ROSServiceController(){

}

bool ROSServiceController::Initialize(){

    LOG_DEBUG("ROSServiceController::Initialize start ");    
    
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
        return GetContainer()->StartContainer(group);
        if (GetContainer()->ContainerSupported() == true){
            return GetContainer()->StartContainer(group);
        }
    }
    return false;
}

bool ROSServiceController::StopGroup(const std::string& group){
    if (!group.empty()){
        if (GetContainer()->ContainerSupported() == true){
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
    
    LOG_DEBUG("ROSServiceController::StartService start " << service->GetName());

    if (service == nullptr) {
        LOG_ERROR("Received null service in StartService.");
        return false;
    }

    std::string basecommand = GetContainer()->BaseRos2Command("");
    std::string command;

    // Assuming launchByFile, GetLauchFile, and GetPackageName are member functions or variables.
    // If not, you need to implement or correct them.
    std::string launchFile = service->GetLaunchFile();
    if (launchFile.empty()){
        command = basecommand + "bash -c \" ros2 run " + service->GetPackage() + " " + service->GetPackage() + " 2>&1 & \"";
    }else{
        command = basecommand + "bash -c \" ros2 launch " + service->GetPackage() + " " + service->GetLaunchFile() + " 2>&1 & \"";
    }

    //int32_t result = system(command.c_str());

    std::regex re(R"(process started with pid \[(\d+)\])");
    std::string exeresult = HelperUtils::ExecCommand(command, re);
    std::vector<std::string> lines = HelperUtils::SplitLines(exeresult);
    /* parse exe command result 
     * process started with pid [pid]
     */
    if (lines.size() > 0){
        std::smatch match;

        for (const auto& line : lines) {
            if (std::regex_search(line, match, re)) {
                // match[1] contains the PID as a string
                uint64_t pid = std::stoi(match[1].str());
                service->SetServiceState(ServiceState::RUNNING);
                service->SetPID(pid);
                LOG_DEBUG("StartService for  :: " << service->GetPackage() << ", pid = " << pid);
                return true;
            }
        }
    }else{
        service->SetServiceState(ServiceState::NOT_RUNNING);
        LOG_ERROR("StartService failed :: " << command);
        return false;
    }

    return false;
}

bool ROSServiceController::StopService(std::shared_ptr<Service> service){
    uint64_t pid = service->GetPID();
    
    if (pid == 0){
        pid = GetPID(service); // Get the PID of the service if needed
    }

    if (pid == 0) {
        LOG_ERROR("Service " << service->GetName() << " is not running.");
        return false; // Service is not running
    }
    // Stop Child Service --> Means to stop a specific container in the pod
    LOG_DEBUG("ROSServiceController::StopChildService start " << pid);

    // Here we can use kubectl command to stop the container
    // For example, using system command to run kubectl
    std::string basecommand = GetContainer()->BaseCommand("");

    std::string command = basecommand + " kill " + std::to_string(pid);

    int32_t result = system(command.c_str());

    if (result == 0){
        LOG_DEBUG("StopService success :: " << command);
        service->SetServiceState(ServiceState::NOT_RUNNING);
        return true;
    }else{
        LOG_ERROR("StopService error : " << command);
    }

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

bool ROSServiceController::GetNodeLists(std::shared_ptr<Service> service){
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

uint64_t ROSServiceController::GetPID(std::shared_ptr<Service> service){

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
    LOG_DEBUG("Processing service start request for " << message->service_list().size() << " services.");

    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStartRequest.");
        return false;
    }
    std::vector<std::string> service_list = message->service_list();
    bool is_group = message->is_group();
    bool result = false;

    if (GetContainer()->ContainerSupported() == false){
        is_group = false;
    }

    if (service_list.empty()) {
        LOG_ERROR("No services provided to start.");
        result = false;
    }
    else{
        LOG_DEBUG("Starting services: " << (is_group ? "Group" : "Individual") << " - " << service_list.size() << " services.");
        if (is_group) {
            for (const auto& service : service_list) {
                result = StartGroup(service);
            }
        } else {
            for (const auto& service : service_list) {
                result = StartService(service);
            }
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

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();

    return transmitter->SendMessage(MessageType::START_SERVICE_RESPONSE, 
                                    reinterpret_cast<void*>(&start_service_response_));
}

bool ROSServiceController::ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message){
    LOG_DEBUG("Processing service stop request for " << message->service_list().size() << " services.");

    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStopRequest.");
        return false;
    }
    std::vector<std::string> service_list = message->service_list();
    bool is_group = message->is_group();
    bool result = false;

    if (GetContainer()->ContainerSupported() == false){
        is_group = false;
    }

    if (service_list.empty()) {
        LOG_ERROR("No services provided to stop.");
        result = false;
    }
    else{
        LOG_DEBUG("Starting services: " << (is_group ? "Group" : "Individual") << " - " << service_list.size() << " services.");

        if (is_group) {
            for (const auto& service : service_list) {
                result = StopGroup(service);
            }
        } else {
            for (const auto& service : service_list) {
                result = StopService(service);
            }
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

    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();

    return transmitter->SendMessage(MessageType::STOP_SERVICE_RESPONSE, 
                                    reinterpret_cast<void*>(&stop_service_response_));
}

    
} // namespace adsw_manager
} // namespace ros2
