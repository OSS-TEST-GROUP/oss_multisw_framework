#ifndef ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_
#define ROS2_ADSW_MANAGER_ADSW_MANAGER_HPP_

#include "transmitter/DDSTransmitter.hpp"
#include "service/ServiceRegistry.hpp"
#include "monitor/ServiceMonitor.hpp"
#include "control/ROSServiceController.hpp"
#include "monitor/ROSServiceDiscovery.hpp"

namespace ros2 {
namespace adsw_manager {

class ADSWManager {
  public:
    ADSWManager();
    virtual ~ADSWManager();

    static ADSWManager* Get();

    bool Initialize();

    bool Start();
    void Stop();

    std::shared_ptr<DDSTransmitter> GetTransmitter();
    std::shared_ptr<ServiceRegistry> GetServiceRegistry();
    std::shared_ptr<ServiceMonitor> GetServiceMonitor();
    std::shared_ptr<ROSServiceController> GetServiceController();
    std::shared_ptr<ROSServiceDiscovery> GetServiceDiscovery();


  private:
    static std::shared_ptr<ADSWManager> getShared();

  private:
    std::shared_ptr<DDSTransmitter> dds_transmitter_;
    std::shared_ptr<ServiceRegistry> service_registry_;
    std::shared_ptr<ServiceMonitor> service_monitor_;
    std::shared_ptr<ROSServiceController> service_controller_;
};

} // namespace adsw_manager
} // namespace ros2

#endif
