#ifndef ROS2_ADSW_MANAGER_SERIVCE_TYPE_COVERT_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_TYPE_COVERT_HPP_

#include "Container.hpp"

namespace ros2 {
namespace adsw_manager {

#include "lifecycle/LifecycleCommon.hpp"

class TypeConvert {
  public:
    TypeConvert();
    virtual ~TypeConvert();

    static ServiceType ToServiceType(lifecyclemgr::ServiceType type){

        switch (type){
            case lifecyclemgr::ServiceType::NORMAL : return ServiceType::NORMAL;
            case lifecyclemgr::ServiceType::ONESHOT : return ServiceType::ONESHOT;
        }
        return ServiceType::NORMAL;
    }


    static lifecyclemgr::ServiceType ToServiceType(ServiceType type){

        switch (type){
            case ServiceType::NORMAL : return lifecyclemgr::ServiceType::NORMAL;
            case ServiceType::ONESHOT : return lifecyclemgr::ServiceType::ONESHOT;
        }
        return lifecyclemgr::ServiceType::NORMAL;
    }

    static ServiceState ToServiceState(lifecyclemgr::ServiceState state){
        switch (state){
            case lifecyclemgr::ServiceState::NOT_RUNNING : return ServiceState::NOT_RUNNING;
            case lifecyclemgr::ServiceState::INITIALIZING : return ServiceState::INITIALIZING;
            case lifecyclemgr::ServiceState::RUNNING : return ServiceState::RUNNING;
            case lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE : return ServiceState::RUNNING_NO_RESPONSE;
            case lifecyclemgr::ServiceState::TERMINATING : return ServiceState::TERMINATING;
        }
        return ServiceState::NOT_RUNNING;
    }


    static lifecyclemgr::ServiceState ToServiceState(ServiceState state){
        switch (state){
            case ServiceState::NOT_RUNNING : return lifecyclemgr::ServiceState::NOT_RUNNING;
            case ServiceState::INITIALIZING : return lifecyclemgr::ServiceState::INITIALIZING;
            case ServiceState::RUNNING : return lifecyclemgr::ServiceState::RUNNING;
            case ServiceState::RUNNING_NO_RESPONSE : return lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE;
            case ServiceState::TERMINATING : return lifecyclemgr::ServiceState::TERMINATING;
        }
        return lifecyclemgr::ServiceState::NOT_RUNNING;
    }    

    static RecoveryPolicy ToRecoveryPolicy(lifecyclemgr::RecoveryPolicy type){

        switch (type){
            case lifecyclemgr::RecoveryPolicy::R_DEFAULT : return RecoveryPolicy::R_DEFAULT;
            case lifecyclemgr::RecoveryPolicy::ALWAYS : return RecoveryPolicy::ALWAYS;
            case lifecyclemgr::RecoveryPolicy::ON_FAILURE : return RecoveryPolicy::ON_FAILURE;
            case lifecyclemgr::RecoveryPolicy::ON_SUCCESS : return RecoveryPolicy::ON_SUCCESS;
            case lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING : return RecoveryPolicy::ON_NOT_RESPONDING;
            case lifecyclemgr::RecoveryPolicy::MANUAL : return RecoveryPolicy::MANUAL;
        }
        return RecoveryPolicy::R_DEFAULT;
    }    
    static lifecyclemgr::RecoveryPolicy ToRecoveryPolicy(RecoveryPolicy type){

        switch (type){
            case RecoveryPolicy::R_DEFAULT : return lifecyclemgr::RecoveryPolicy::R_DEFAULT;
            case RecoveryPolicy::ALWAYS : return lifecyclemgr::RecoveryPolicy::ALWAYS;
            case RecoveryPolicy::ON_FAILURE : return lifecyclemgr::RecoveryPolicy::ON_FAILURE;
            case RecoveryPolicy::ON_SUCCESS : return lifecyclemgr::RecoveryPolicy::ON_SUCCESS;
            case RecoveryPolicy::ON_NOT_RESPONDING : return lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING;
            case RecoveryPolicy::MANUAL : return lifecyclemgr::RecoveryPolicy::MANUAL;
        }
        return lifecyclemgr::RecoveryPolicy::R_DEFAULT;
    }    

    static OsType ToOsType(lifecyclemgr::OsType os) {
        switch (os) {
            case lifecyclemgr::OsType::ROS2 : return OsType::ROS2;
            case lifecyclemgr::OsType::AGL : return OsType::AGL;
            case lifecyclemgr::OsType::AUTOSAR : return OsType::AUTOSAR;
        }
    }
    static lifecyclemgr::OsType ToOsType(OsType os) {
        switch (os) {
            case OsType::ROS2 : return lifecyclemgr::OsType::ROS2;
            case OsType::AGL : return lifecyclemgr::OsType::AGL;
            case OsType::AUTOSAR : return lifecyclemgr::OsType::AUTOSAR;
        }
    }

    // F013: Container enum bridge between adsw_manager local enum and the IDL.
    // Note local enum is `UNKNOWN/K3S/DOCKER/DOCKER_COMPOSE`; IDL uses
    // `CT_UNKNOWN/DOCKER/DOCKER_COMPOSE/K3S` (slightly different ordering).
    static lifecyclemgr::ContainerType ToContainerType(ContainerType t) {
        switch (t) {
            case ContainerType::DOCKER         : return lifecyclemgr::ContainerType::DOCKER;
            case ContainerType::DOCKER_COMPOSE : return lifecyclemgr::ContainerType::DOCKER_COMPOSE;
            case ContainerType::K3S            : return lifecyclemgr::ContainerType::K3S;
            case ContainerType::UNKNOWN        :
            default                            : return lifecyclemgr::ContainerType::CT_UNKNOWN;
        }
    }
};


} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_SERIVCE_TYPE_COVERT_HPP_
