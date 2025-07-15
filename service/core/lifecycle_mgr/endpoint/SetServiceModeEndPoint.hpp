
#ifndef LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "transmitter/DDSMessageEndPoint.hpp"

#include "lifecycle/SetServiceModeRequestPubSubTypes.hpp"

namespace lifecycle_manager {

using namespace lifecyclemgr;

class SetServiceModeEndPoint : public DDSMessageEndPoint {
  public:
    SetServiceModeEndPoint(std::string topic);
    virtual ~SetServiceModeEndPoint();

    virtual void  OnDataAvailable() override;

    virtual void OnDataPublished(bool ret) override;

  private:
    SetServiceModeRequest data_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_SET_SERVICE_MODE_MESSAGE_ENDPOINT_HPP_
