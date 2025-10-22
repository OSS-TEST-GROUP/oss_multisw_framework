#ifndef OTA_MGR_COMMON_SERVICE_H_
#define OTA_MGR_COMMON_SERVICE_H_

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
   };
   
#endif  // OTA_MGR_COMMON_SERVICE_H_