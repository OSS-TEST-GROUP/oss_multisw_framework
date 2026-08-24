
#ifndef LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class LifecycleDiagReportEndPoint : public DDSMessageEndPoint {
  public:
    LifecycleDiagReportEndPoint(MessageType msgtype, std::string topic);
    ~LifecycleDiagReportEndPoint() override;

    void OnDataAvailable() override;

    void OnDataPublished(bool ret) override;

  private:
    LifecycleDiagReport data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_
