#ifndef LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
#define LIFECYCLE_START_SERVICE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/StartServiceRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class StartServiceEndPoint : public DDSMessageEndPoint {
  public:
    StartServiceEndPoint(std::string topic);
    virtual ~StartServiceEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    StartServiceRequest req_;
    StartServiceResponse resp_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_START_SERVICE_ENDPOINT_HPP_
