#ifndef ROS2_ADSW_MANAGER_CONTROL_CONTAINER_SERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_CONTAINER_SERVICE_CONTROLLER_HPP_

#include <memory>
#include <string>

#include "ServiceController.hpp"
#include "ContainerController.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

namespace ros2 {
namespace adsw_manager {

class ContainerServiceController  : public ServiceController {

  public:
    ContainerServiceController();
    ~ContainerServiceController();

    virtual bool Initialize();
    virtual void Finalize();

    virtual bool StartGroup(const std::string& group);
    virtual bool StopGroup(const std::string& group);

    virtual bool StartService(const std::string& service_name);
    virtual bool StopService(const std::string& service_name);


    virtual bool StartService(std::shared_ptr<Service> service);
    virtual bool StopService(std::shared_ptr<Service> service);

    bool ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message);
    bool ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message);

    std::shared_ptr<ContainerController> GetContainer() {return container_controller_;}

  private:
    std::shared_ptr<ContainerController>  container_controller_;

    lifecyclemgr::StartServiceResponse start_service_response_;
    lifecyclemgr::StopServiceResponse stop_service_response_;

};

} // namespace adsw_manager
} // namespace ros2

#endif
