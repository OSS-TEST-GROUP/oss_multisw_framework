#include "ContainerController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {


ContainerController::ContainerController(const std::string& deployment_file_path,
                        const std::string& pod_name,
                        const std::string& namespace_,
                        const std::string& container_name) 
    : deployment_file_path_(deployment_file_path),
      pod_name_(pod_name),
      namespace_(namespace_),
      container_name_(container_name) {
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
    if (k3s_container_ == true){
        std::string command = "kubectl apply -f " + deployment_file_path_;
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
    if (k3s_container_ == true){
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

    if (k3s_container_ == true){

        std::string command = "kubectl get pod " + pod_name_ + " -n " + namespace_ + " -o jsonpath='{.status.containerStatuses[?(@.name==\"" + container_name + "\")].state}'";

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

    if (k3s_container_ == true){

        std::string command = "kubectl logs " + pod_name_ + " -n " + namespace_ + " -c " + container_name;

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

std::string ContainerController::BaseCommand() {

    if (k3s_container_ == true){
        std::string command = "kubectl exec -n " + GetNamespace() + " " + GetPodName() + " -- ";
        LOG_DEBUG("Base command for container: " << command);
        return command;
    }
    return "";
}

std::string ContainerController::BaseRos2Command() {

    if (k3s_container_ == true){
        std::string command = "kubectl exec -n " + GetNamespace() + " " + GetPodName() + " -- ";
        command += "bash -c \"source /opt/ros/humble/setup.bash && ";
        LOG_DEBUG("Base command for container: " << command);
        return command;
    }
    return "";
}

} // namespace adsw_manager
} // namespace ros2
