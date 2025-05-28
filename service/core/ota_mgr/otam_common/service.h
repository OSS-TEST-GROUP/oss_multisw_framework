#ifndef OTA_MGR_COMMON_SERVICE_H_
#define OTA_MGR_COMMON_SERVICE_H_

/**
 * @brief Base interface for all OTA Manager services.
 * This abstract class defines the common lifecycle methods (Initialize, Start, Stop)
 * that all services within the OTA Manager component must implement.
 */
 class Service {
    public:
     virtual ~Service() = default;
     virtual void Initialize() = 0;
     virtual void Start() = 0;
     virtual void Stop() = 0;
   };
   
#endif  // OTA_MGR_COMMON_SERVICE_H_