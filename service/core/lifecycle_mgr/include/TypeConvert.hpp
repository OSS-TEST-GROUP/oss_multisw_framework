#ifndef LIFECYCLE_TYPE_CONVERT_HPP_
#define LIFECYCLE_TYPE_CONVERT_HPP_

#include "lifecycle/LifecycleCommon.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

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


    static ServiceMode ToServiceMode(lifecyclemgr::ServiceMode type){

        switch (type){
            case lifecyclemgr::ServiceMode::NORMAL : return ServiceMode::NORMAL;
            case lifecyclemgr::ServiceMode::OTA_MODE : return ServiceMode::OTA_MODE;
        }
        return ServiceMode::NORMAL;
    }

    static lifecyclemgr::ServiceMode ToServiceMode(ServiceMode type){

        switch (type){
            case ServiceMode::NORMAL : return lifecyclemgr::ServiceMode::NORMAL;
            case ServiceMode::OTA_MODE : return lifecyclemgr::ServiceMode::OTA_MODE;
        }
        return lifecyclemgr::ServiceMode::NORMAL;
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
};


} // namespace lifecycle_manager

#endif // LIFECYCLE_TYPE_CONVERT_HPP_
