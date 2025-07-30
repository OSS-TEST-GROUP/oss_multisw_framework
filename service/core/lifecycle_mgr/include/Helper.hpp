#ifndef LIFECYCLE_MANAGER_HELPER_HPP_
#define LIFECYCLE_MANAGER_HELPER_HPP_

#include <string>

#include "MessageType.hpp"
#include "RecoveryPolicy.hpp"
#include "ServiceMode.hpp"
#include "ServiceState.hpp"
#include "ServiceType.hpp"
#include "ServiceControlAction.hpp"
#include "ManagerState.hpp"

namespace lifecycle_manager {

class Helper {
  public:
    Helper();
    virtual ~Helper();

    static std::string PubSubTypeStr(PubSubTYpe type){
        switch (type){
            case PubSubTYpe::PUBSUB_NONE : return "None";
            case PubSubTYpe::PUBLISH_ONLY : return "Publish";
            case PubSubTYpe::SUBSCRIBE_ONLY : return "Subscriber";
            case PubSubTYpe::PUBSUB_BOTH : return "PubAndSub";
        }
        return "";
    }

    static std::string RecoveryStr(RecoveryPolicy type){
        switch (type){
            case RecoveryPolicy::R_DEFAULT : return "Default";
            case RecoveryPolicy::ALWAYS : return "Always";
            case RecoveryPolicy::ON_FAILURE : return "OnFailure";
            case RecoveryPolicy::ON_SUCCESS : return "OnSuccess";
            case RecoveryPolicy::ON_NOT_RESPONDING : return "OnNotRespond";
            case RecoveryPolicy::MANUAL : return "Manual";
        }
        return "";
    }

    static std::string ActionStr(ServiceControlAction type){
        switch (type){
            case ServiceControlAction::NONE : return "None";
            case ServiceControlAction::START : return "Start";
            case ServiceControlAction::STOP : return "Stop";
        }
        return "";
    }      

    static std::string SvcModeStr(ServiceMode type){
        switch (type){
            case ServiceMode::NORMAL : return "Normal";
            case ServiceMode::OTA_MODE : return "OTA-Mode";
        }
        return "";
    }   

    static std::string StateStr(ServiceState type){
        switch (type){
            case ServiceState::NOT_RUNNING : return "NotRunning";
            case ServiceState::INITIALIZING : return "Initializing";
            case ServiceState::RUNNING : return "Running";
            case ServiceState::RUNNING_NO_RESPONSE : return "NoResponse";
            case ServiceState::TERMINATING : return "Terminating";

        }
        return "";
    }   


    static std::string SvcTypeStr(ServiceType type){
        switch (type){
            case ServiceType::NORMAL : return "Normal";
            case ServiceType::ONESHOT : return "OneShot";
        }
        return "";
    }

    static std::string ManagerStateStr(ManagerState type){
        switch (type){
            case ManagerState::MANAGER_INIT : return "INIT";
            case ManagerState::MANAGER_STARTED : return "STARTED";
            case ManagerState::MANAGER_RUNNING : return "RUNNING";
            case ManagerState::MANAGER_PASUE : return "PAUSE";
            case ManagerState::MANAGER_TERMINATE : return "TERMINATE";
        }
        return "";
    }   

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MANAGER_HELPER_HPP_
