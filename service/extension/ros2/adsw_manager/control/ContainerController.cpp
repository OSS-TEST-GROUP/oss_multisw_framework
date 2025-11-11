#include "ContainerController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

const std::string ADSW_ROS2_CONTAINER_NAME = "oss-mmf-ext-ros2";

ContainerController::ContainerController() {
    // Constructor for ContainerController
    // Initialize the deployment file path, pod name, namespace, and container name
    // This is where you can set up any necessary configurations or resources
    LOG_DEBUG("ContainerController create ");
}

ContainerController::~ContainerController(){

}

bool ContainerController::Initialize(){
    k3s_container_ = false;
    return true;
}

void ContainerController::Finalize(){
}

bool ContainerController::StartContainer(const std::string& container_name){

    // Start Container --> Means to start POD in Kubernetes
    LOG_DEBUG("ContainerController::StartContainer start " << container_name);
    
    // Here we can use kubectl command to start the container
    // For example, using system command to run kubectl

    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){

        std::string command = "kubectl apply -f " + container->GetDeployFile();
        int result = system(command.c_str());

        if (result == 0) {
            LOG_DEBUG("ContainerController::StartContainer success " << container_name);
            return true;
        } else {
            LOG_ERROR("ContainerController::StartContainer failed " << container_name);
            return false;
        }
        LOG_DEBUG("ContainerController::StartContainer end " << container_name);
        return true;
    }else{
        LOG_ERROR("ContainerController::StartContainer not k3s support " << container_name);
    }
    return false;
}

bool ContainerController::StopContainer(const std::string& container_name){

    // Stop Container --> Means to stop POD in Kubernetes
    LOG_DEBUG("ContainerController::StopContainer start " << container_name);
    
    // Here we can use kubectl command to stop the container
    // For example, using system command to run kubectl
    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){
        std::string command = "kubectl delete pod " + container_name;
        int result = system(command.c_str());

        if (result == 0) {
            LOG_DEBUG("ContainerController::StopContainer success " << container_name);
            return true;
        } else {
            LOG_ERROR("ContainerController::StopContainer failed " << container_name);
            return false;
        }
        LOG_DEBUG("ContainerController::StopContainer end " << container_name);
        return true;
    }else{
        LOG_ERROR("ContainerController::StopContainer not k3s support " << container_name);
    }
    return false;
}

bool ContainerController::GetContainerStatus(const std::string& container_name) {

    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){
        std::string command = "kubectl get pod " + container->GetPodName() + " -n " + container->GetNamespace() + " -o jsonpath='{.status.containerStatuses[?(@.name==\"" + container_name + "\")].state}'";

        int result = system(command.c_str());

        if (result == 0) {
            LOG_DEBUG("ContainerController::GetContainerStatus success " << container_name);
            return true;
        } else {
            LOG_ERROR("ContainerController::GetContainerStatus failed " << container_name);
            return false;
        }       
    }else{
        LOG_ERROR("ContainerController::GetContainerStatus not k3s support " << container_name);
    }
    return false;
}

bool ContainerController::GetContainerLogs(const std::string& container_name) {

    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){

        std::string command = "kubectl logs " + container->GetPodName() + " -n " + container->GetNamespace() + " -c " + container_name;

        int result = system(command.c_str());

        if (result == 0) {
            LOG_DEBUG("ContainerController::GetContainerLogs success " << container_name);
            return true;
        } else {
            LOG_ERROR("ContainerController::GetContainerLogs failed " << container_name);
            return false;
        }       
    }else{
        LOG_ERROR("ContainerController::GetContainerLogs not k3s support " << container_name);
    }
    return false;
}

std::shared_ptr<Container> ContainerController::GetContainer(const std::string& container_name) {
        
    if (k3s_container_ == false) {
        LOG_ERROR("ContainerController::GetContainer not k3s support " << container_name);
        return nullptr;
    }
    for (const auto& container : containers_) {
            
        if (container->GetName() == container_name) {
                return container;
        }
    }
    LOG_ERROR("ContainerController::GetContainer not found " << container_name);
    return nullptr; // Return nullptr if the container is not found
}

bool ContainerController::AddContainer(std::shared_ptr<Container> container){
    if (container == nullptr) {
        LOG_ERROR("ContainerController::AddContainer received null container.");
        return false;
    }

    // Check if the container already exists
    for (const auto& existing_container : containers_) {
        if (existing_container->GetName() == container->GetName()) {
            LOG_ERROR("ContainerController::AddContainer container already exists: " << container->GetName());
            return false; // Container already exists
        }
    }

    containers_.push_back(container);
    LOG_DEBUG("ContainerController::AddContainer added container: " << container->GetName());
    return true;

}



std::string ContainerController::BaseCommand(const std::string& container_name) {

    if (container_name == ADSW_ROS2_CONTAINER_NAME){
        LOG_DEBUG("Container is ADSW ROS2 : " << container_name);
        return ""; // this shall be controlled as ros2 base command
    }

    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){
        std::string command = "kubectl exec -n " + container->GetNamespace() + " " + container->GetPodName() + " -- ";
        LOG_DEBUG("Base command for container: " << command);
        return command;
    }
    return "";
}

std::string ContainerController::BaseRos2Command(const std::string& container_name) {

    if (container_name == ADSW_ROS2_CONTAINER_NAME){
        LOG_DEBUG("Container is ADSW ROS2 : " << container_name);        
        return ""; // this shall be controlled as ros2 base command
    }

    std::shared_ptr<Container> container = GetContainer(container_name);

    if (container != nullptr){
        std::string command = "kubectl exec -n " + container->GetNamespace() + " " + container->GetPodName() + " -- ";
        command += "bash -c \"source /opt/ros/humble/setup.bash && ";
        LOG_DEBUG("Base command for container: " << command);
        return command;
    }
    return "";
}

bool ContainerController::ContainerSupported() {
    // Check if the container is supported
    // This can be based on the container name or other criteria
    return k3s_container_;
}

} // namespace adsw_manager
} // namespace ros2
