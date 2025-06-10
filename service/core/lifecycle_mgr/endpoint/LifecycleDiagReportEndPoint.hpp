
#ifndef LIFECYCLE_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"

namespace lifecycle_manager {

class LifecycleDiagReportEndPoint : public DDSMessageEndPoint {
  public:
    LifecycleDiagReportEndPoint(std::string topic, std::string type);
    virtual ~LifecycleDiagReportEndPoint();

    void OnDataAvailable() override;
    void* GetTopicBuffer() override;

  private:
    LifecycleDiagReport data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_ENDPOINT_HPP_
