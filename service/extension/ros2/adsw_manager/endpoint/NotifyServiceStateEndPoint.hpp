#ifndef LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
#define LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>
\

#include "transmitter/DDSMessageEndpoint.hpp"
#include "lifecycle/NotifyServiceStatePubSubTypes.hpp"

namespace ros2 {
namespace adsw_manager {


class NotifyServiceStateEndPoint : public DDSMessageEndpoint {
  public:
    NotifyServiceStateEndPoint(std::string topic);
    virtual ~NotifyServiceStateEndPoint();

    virtual std::string GetTopic();
    virtual std::string GetTypeName();
    virtual std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable();

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    

};


} // namespace adsw_manager
} // namespace ros2

#endif // LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
