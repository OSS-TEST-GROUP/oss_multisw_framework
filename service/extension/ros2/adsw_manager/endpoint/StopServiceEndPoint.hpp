#ifndef LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StopServiceRequestPubSubTypes.hpp"

namespace ros2 {
namespace adsw_manager {

class StopServiceEndPoint : public DDSMessageEndpoint {
  public:
    StopServiceEndPoint(std::string topic);
    virtual ~StopServiceEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    StopServiceRequest req_;
    StopServiceResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_STOP_SERVICE_STATE_ENDPOINT_HPP_
