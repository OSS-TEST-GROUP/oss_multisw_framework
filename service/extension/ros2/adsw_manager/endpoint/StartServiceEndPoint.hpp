#ifndef LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_START_SERVICE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/StartServiceRequestPubSubTypes.hpp"

namespace ros2 {
namespace adsw_manager {

using namespace lifecyclemgr;

class StartServiceEndPoint : public DDSMessageEndpoint {
  public:
    StartServiceEndPoint(std::string topic);
    virtual ~StartServiceEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    
  
    StartServiceRequest req_;
    StartServiceResponse resp_;

};

} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
