
#ifndef LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/LifecycleDiagReportPubSubTypes.hpp"

namespace lifecycle_manager {

class LifecycleDiagReportEndPoint : public DDSMessageEndPoint {
  public:
    LifecycleDiagReportEndPoint(std::string topic);
    virtual ~LifecycleDiagReportEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    LifecycleDiagReport data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DIAG_REPORT_MESSAGE_ENDPOINT_HPP_
