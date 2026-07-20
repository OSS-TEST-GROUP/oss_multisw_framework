#ifndef GENERAL_ADSW_MANAGER_CONTROL_SERVICE_CONTROLLER_HPP_
#define GENERAL_ADSW_MANAGER_CONTROL_SERVICE_CONTROLLER_HPP_

#include <string>
#include <memory>

#include "service/Service.hpp"

namespace general {
namespace adsw_manager {

class ServiceController {
  public:

    virtual bool Initialize() = 0;
    virtual void Finalize() = 0;

    virtual bool StartGroup(const std::string& group) = 0;
    virtual bool StopGroup(const std::string& group) = 0;

    virtual bool StartService(const std::string& service_name) = 0;
    virtual bool StopService(const std::string& service_name) = 0;

    virtual bool StartService(std::shared_ptr<Service> service) = 0;
    virtual bool StopService(std::shared_ptr<Service> service) = 0;

  private:
};

} // namespace adsw_manager
} // namespace general

#endif
