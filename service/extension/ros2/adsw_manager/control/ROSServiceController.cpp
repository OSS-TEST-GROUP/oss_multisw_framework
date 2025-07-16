#include "ROSServiceController.hpp"

#include "ADSWManager.hpp"
#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "logger/Log.hpp"

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
    if (ros2node_controller_ != nullptr){
        ros2node_controller_->Initialize();
    }

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
    if (ros2node_controller_ != nullptr){
        ros2node_controller_->Finalize();
    }
}

bool ROSServiceController::StartGroup(const std::string& group){

}

bool ROSServiceController::StopGroup(const std::string& group){

}


bool ROSServiceController::StartService(const std::string& service_name){

}

bool ROSServiceController::StopService(const std::string& service_name){

}


bool ROSServiceController::StartService(std::shared_ptr<Service> service){
    if (service == nullptr) {
        LOG_ERROR("Received null service in StartService.");
        return false;
    }

    LOG_DEBUG("Starting service: " << service->GetName());



}

bool ROSServiceController::StopService(std::shared_ptr<Service> service){

}

uint64_t ROSServiceController::GetPID(std::shared_ptr<Service> service){

    return 1234; // temporary PID for demonstration purposes
    if (service == nullptr) {
        LOG_ERROR("Received null service in GetPID.");
        return 0;
    }

    // Here we can implement logic to get the PID of the service
    // For example, if the service is a ROS2 node, we can use ros2node_controller_ to get the PID
    // If the service is a container, we can use container_controller_ to get the PID

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
