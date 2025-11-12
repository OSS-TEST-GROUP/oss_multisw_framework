#ifndef OTA_MGR_COMMON_SERVICE_H_
#define OTA_MGR_COMMON_SERVICE_H_

#include <string>

/**
 * @brief Service state enumeration
 * Represents the lifecycle state of an OTA Manager service.
 */
enum class ServiceState {
    UNINITIALIZED,  // Service has not been initialized
    INITIALIZED,    // Service has been initialized but not started
    STARTED,        // Service is running normally
    STOPPED,        // Service has been stopped
    ERROR           // Service encountered an error
};

enum class OtamError {
    SUCCEEDED,
    FAILED,
    UNKNOWN
};

/**
 * @brief Convert ServiceState enum to string representation
 */
inline std::string ServiceStateToString(ServiceState state) {
    switch (state) {
        case ServiceState::UNINITIALIZED:
            return "UNINITIALIZED";
        case ServiceState::INITIALIZED:
            return "INITIALIZED";
        case ServiceState::STARTED:
            return "STARTED";
        case ServiceState::STOPPED:
            return "STOPPED";
        case ServiceState::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Base interface for all OTA Manager services.
 * This abstract class defines the common lifecycle methods (Initialize, Start, Stop)
 * that all services within the OTA Manager component must implement.
 */
 class OtamService {
    public:
     virtual ~OtamService() = default;
     virtual void Initialize() = 0;
     virtual void Start() = 0;
     virtual void Stop() = 0;
     virtual ServiceState State() { return state_; }
     virtual std::string Name() { return name_; }
    protected:
     ServiceState state_{ServiceState::UNINITIALIZED};
     std::string name_;
   };
   
#endif  // OTA_MGR_COMMON_SERVICE_H_