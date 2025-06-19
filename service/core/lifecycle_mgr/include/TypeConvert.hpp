#ifndef LIFECYCLE_TYPE_CONVERT_HPP_
#define LIFECYCLE_TYPE_CONVERT_HPP_

#include "lifecycle/LifecycleCommon.hpp"

namespace lifecycle_manager {

class TypeConvert {
  public:
    TypeConvert();
    virtual ~TypeConvert();

    static ServiceType ToServiceType(::ServiceType type){

        switch (type){
            case ::ServiceType::NORMAL : return ServiceType::NORMAL;
            case ::ServiceType::ONESHOT : return ServiceType::ONESHOT;
        }
        return ServiceType::NORMAL;
    }


    static ::ServiceType ToServiceType(ServiceType type){

        switch (type){
            case ServiceType::NORMAL : return ::ServiceType::NORMAL;
            case ServiceType::ONESHOT : return ::ServiceType::ONESHOT;
        }
        return ::ServiceType::NORMAL;
    }

    static ServiceState ToServiceState(::ServiceState state){
        switch (state){
            case ::ServiceState::NOT_RUNNING : return ServiceState::NOT_RUNNING;
            case ::ServiceState::INITIALIZING : return ServiceState::INITIALIZING;
            case ::ServiceState::RUNNING : return ServiceState::RUNNING;
            case ::ServiceState::RNNING_NO_REPONSE : return ServiceState::RUNNING_NO_RESPONSE;
            case ::ServiceState::TERMINATING : return ServiceState::TERMINATING;
        }
        return ServiceState::NOT_RUNNING;
    }


    static ::ServiceState ToServiceState(ServiceState state){
        switch (state){
            case ServiceState::NOT_RUNNING : return ::ServiceState::NOT_RUNNING;
            case ServiceState::INITIALIZING : return ::ServiceState::INITIALIZING;
            case ServiceState::RUNNING : return ::ServiceState::RUNNING;
            case ServiceState::RUNNING_NO_RESPONSE : return ::ServiceState::RNNING_NO_REPONSE;
            case ServiceState::TERMINATING : return ::ServiceState::TERMINATING;
        }
        return ::ServiceState::NOT_RUNNING;
    }    


    static ServiceMode ToServiceMode(::ServiceMode type){

        switch (type){
            case ::ServiceMode::NORMAL : return ServiceMode::NORMAL;
            case ::ServiceMode::OTA_MODE : return ServiceMode::OTA_MODE;
        }
        return ServiceMode::NORMAL;
    }

    static ::ServiceMode ToServiceMode(ServiceMode type){

        switch (type){
            case ServiceMode::NORMAL : return ::ServiceMode::NORMAL;
            case ServiceMode::OTA_MODE : return ::ServiceMode::OTA_MODE;
        }
        return ::ServiceMode::NORMAL;
    }

    static RecoveryPolicy ToRecoveryPolicy(::RecoveryPolicy type){

        switch (type){
            case ::RecoveryPolicy::R_DEFAULT : return RecoveryPolicy::R_DEFAULT;
            case ::RecoveryPolicy::ALWAYS : return RecoveryPolicy::ALWAYS;
            case ::RecoveryPolicy::ON_FAILURE : return RecoveryPolicy::ON_FAILURE;
            case ::RecoveryPolicy::ON_SUCCESS : return RecoveryPolicy::ON_SUCCESS;
            case ::RecoveryPolicy::ON_NOT_RESPONDING : return RecoveryPolicy::ON_NOT_RESPONDING;
            case ::RecoveryPolicy::MANUAL : return RecoveryPolicy::MANUAL;
        }
        return RecoveryPolicy::R_DEFAULT;
    }    
    static ::RecoveryPolicy ToRecoveryPolicy(RecoveryPolicy type){

        switch (type){
            case RecoveryPolicy::R_DEFAULT : return ::RecoveryPolicy::R_DEFAULT;
            case RecoveryPolicy::ALWAYS : return ::RecoveryPolicy::ALWAYS;
            case RecoveryPolicy::ON_FAILURE : return ::RecoveryPolicy::ON_FAILURE;
            case RecoveryPolicy::ON_SUCCESS : return ::RecoveryPolicy::ON_SUCCESS;
            case RecoveryPolicy::ON_NOT_RESPONDING : return ::RecoveryPolicy::ON_NOT_RESPONDING;
            case RecoveryPolicy::MANUAL : return ::RecoveryPolicy::MANUAL;
        }
        return ::RecoveryPolicy::R_DEFAULT;
    }    
};


} // namespace lifecycle_manager

#endif // LIFECYCLE_TYPE_CONVERT_HPP_
