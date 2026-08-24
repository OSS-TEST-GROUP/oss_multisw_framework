#ifndef GENERAL_ADSW_MANAGER_CONTROL_CONTAINER_SERVICE_CONTROLLER_HPP_
#define GENERAL_ADSW_MANAGER_CONTROL_CONTAINER_SERVICE_CONTROLLER_HPP_

#include <memory>
#include <string>

#include "ServiceController.hpp"
#include "ContainerController.hpp"

#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"

namespace general {
namespace adsw_manager {

class ContainerServiceController  : public ServiceController {

  public:
    ContainerServiceController();
    ~ContainerServiceController();

    bool Initialize() override;
    void Finalize() override;

    bool StartGroup(const std::string& group) override;
    bool StopGroup(const std::string& group) override;

    bool StartService(const std::string& service_name) override;
    bool StopService(const std::string& service_name) override;

    bool StartService(std::shared_ptr<Service> service) override;
    bool StopService(std::shared_ptr<Service> service) override;

    bool ProcessServiceStartRequest(lifecyclemgr::StartServiceRequest* message);
    bool ProcessServiceStopRequest(lifecyclemgr::StopServiceRequest* message);

    std::shared_ptr<ContainerController> GetContainer() {return container_controller_;}

  private:
    std::shared_ptr<ContainerController>  container_controller_;

    lifecyclemgr::StartServiceResponse start_service_response_;
    lifecyclemgr::StopServiceResponse stop_service_response_;

};

} // namespace adsw_manager
} // namespace general

#endif
