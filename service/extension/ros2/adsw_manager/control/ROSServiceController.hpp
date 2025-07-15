#ifndef ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_ROSSERVICE_CONTROLLER_HPP_

#include <string>
#include <memory>

#include "ServiceController.hpp"
#include "ROS2NodeController.hpp"
#include "ContainerController.hpp"


#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

namespace ros2 {
namespace adsw_manager {

class ROSServiceController  : public ServiceController {

  public:
    ROSServiceController();
    ~ROSServiceController();

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


    uint64_t GetPID(std::shared_ptr<Service> service);

  private:
    std::shared_ptr<ROS2NodeController>  ros2node_controller_;
    std::shared_ptr<ContainerController>  container_controller_;

};

} // namespace adsw_manager
} // namespace ros2

#endif
