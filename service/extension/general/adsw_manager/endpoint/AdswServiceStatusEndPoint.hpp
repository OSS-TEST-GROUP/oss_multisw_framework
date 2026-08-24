#ifndef LIFECYCLE_ADSW_SERVICE_STATUS_ENDPOINT_HPP_
#define LIFECYCLE_ADSW_SERVICE_STATUS_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndpoint.hpp"
#include "ros2/AdswServiceStatusPubSubTypes.hpp"
#include "MessageTypes.hpp"

namespace general {
namespace adsw_manager {

using namespace adswmgr;

class ADSWServiceStatusEndPoint : public DDSMessageEndpoint {
  public:
    ADSWServiceStatusEndPoint(MessageType msgtype, std::string topic);
    ~ADSWServiceStatusEndPoint() override;

    std::string GetTopic() override;
    std::string GetTypeName() override;
    std::shared_ptr<TypeSupport> GetTypeSupport() override;

    void OnDataAvailable() override;

  private:
    std::string topic_;
    std::shared_ptr<TypeSupport> type_support_;    

};


} // namespace adsw_manager
} // namespace general

#endif // LIFECYCLE_NOTIFY_SERVICE_STATE_ENDPOINT_HPP_
