#include "ROSServiceController.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
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

    container_controller_ = std::make_shared<ContainerController>("deployment_file_path_example.yaml", 
        "pod_name_example", "namespace_example", "container_name_example");
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

}

bool ROSServiceController::StopGroup(const std::string& group){

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

    std::string basecommand = GetContainer()->BaseRos2Command();
    std::string command;

    // Assuming launchByFile, GetLauchFile, and GetPackageName are member functions or variables.
    // If not, you need to implement or correct them.
    bool launchByFile = false; // Placeholder, replace with actual logic
    if (launchByFile){
        command = basecommand + " ros2 launch " + GetPackageName() + " " + GetLauchFile();
    }else{
        command = basecommand + " ros2 run " + GetPackageName();
    }

    int result = system(command.c_str());
    
    if (result == 0) {
        LOG_DEBUG("ROSServiceController::StartService success " << service->GetName());
        return true;
    } else {
        LOG_ERROR("ROSServiceController::StartService failed " << service->GetName());
        return false;
    }
}

bool ROSServiceController::StopService(std::shared_ptr<Service> service){
    uint64_t pid = GetPID(service); // Get the PID of the service if needed

    if (pid == 0) {
        LOG_ERROR("Service " << service->GetName() << " is not running.");
        return false; // Service is not running
    }
    // Stop Child Service --> Means to stop a specific container in the pod
    LOG_DEBUG("ROSServiceController::StopChildService start " << pid);

    // Here we can use kubectl command to stop the container
    // For example, using system command to run kubectl
    std::string basecommand = GetContainer()->BaseCommand();
 
    std::string command = basecommand + " kill " + std::to_string(pid);
    int result = system(command.c_str());
    
    if (result != 0) {
        LOG_ERROR("ROSServiceController::StopChildService failed " << pid);
        return false;
    }
    LOG_DEBUG("ROSServiceController::StopChildService success " << pid);
    return true;    
    
    if (result != 0) {
        LOG_ERROR("ROSServiceController::StopChildService failed " << pid);
        return false;
    }
    LOG_DEBUG("ROSServiceController::StopChildService success " << pid);
    return true;    
}

bool ROSServiceController::GetNodeLists(std::shared_ptr<Service> service){
    if (service == nullptr) {
        LOG_ERROR("Received null service in GetNodeLists.");
        return false;
    }

    // kubectl exec -n default ros2-sample-all -- \
    // bash -c "source /opt/ros/humble/setup.bash && source /ros2_ws/install/setup.bash &&
    // ros2 pkg executables sample_ros2_app

    // sample_ros2_app listener
    // sample_ros2_app talker

    std::string basecommand = GetContainer()->BaseRos2Command();

    std::string command = basecommand + " ros2 pkg executables " + GetPackageName();
    
    std::string exeresult = HelperUtils::ExecCommand(command);

    std::vector<std::string> lines = HelperUtils::SplitLines(exeresult);

    for (auto& line : lines){
        // service->AddNode();
    }

    return true;
}

uint64_t ROSServiceController::GetPID(std::shared_ptr<Service> service){

     if (service == nullptr) {
        LOG_ERROR("Received null service in GetPID.");
        return 0;
    }

    std::string basecommand = GetContainer()->BaseCommand();
    std::string command = basecommand + " ps aux";

    // Command to run inside the container
    try {
        std::string output = HelperUtils::ExecCommand(command);
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
        LOG_ERROR("ContainerController::GetPIDs failed " << basecommand << ": " << e.what());
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
    bool result;

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
    lifecyclemgr::ReturnCode ret_code = lifecyclemgr::ReturnCode::SUCCESS;
    if (!result) {
        ret_code = lifecyclemgr::ReturnCode::FAILED;
        LOG_ERROR("Failed to start service(s).");
    } else {
        ret_code = lifecyclemgr::ReturnCode::SUCCESS;
        LOG_DEBUG("Service(s) started successfully.");
    }

    lifecyclemgr::StartServiceResponse response;
    response.ret_code() = ret_code;

    const char* topic = ADSWManager::Get()->GetTopic(MessageType::START_SERVICE_RESPONSE);
    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

    // Write the response back to the DDS topic
    if (endpoint != nullptr) {
        endpoint->Write(reinterpret_cast<void*>(&response));
    }
    return true;

}

bool ROSServiceController::ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message){
    LOG_DEBUG("Processing service stop request for " << message->service_list().size() << " services.");

    if (message == nullptr) {
        LOG_ERROR("Received null message in ProcessServiceStopRequest.");
        return false;
    }
    std::vector<std::string> service_list = message->service_list();
    bool is_group = message->is_group();
    bool result;

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
    lifecyclemgr::ReturnCode ret_code = lifecyclemgr::ReturnCode::SUCCESS;
    if (!result) {
        ret_code = lifecyclemgr::ReturnCode::FAILED;
        LOG_ERROR("Failed to start service(s).");
    } else {
        ret_code = lifecyclemgr::ReturnCode::SUCCESS;
        LOG_DEBUG("Service(s) started successfully.");
    }

    lifecyclemgr::StopServiceResponse response;
    response.ret_code() = ret_code;

    const char* topic = ADSWManager::Get()->GetTopic(MessageType::STOP_SERVICE_RESPONSE);
    std::shared_ptr<DDSTransmitter> transmitter = ADSWManager::Get()->GetTransmitter();
    std::shared_ptr<DDSMessageEndpoint> endpoint = transmitter->GetMessageEndpoint(topic);

    // Write the response back to the DDS topic
    if (endpoint != nullptr) {
        endpoint->Write(reinterpret_cast<void*>(&response));
    }
    return true;
}

    
} // namespace adsw_manager
} // namespace ros2
