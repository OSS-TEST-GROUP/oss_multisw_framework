
#ifndef LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include "MessageEndPoint.hpp"
#include "DDSPublisher.hpp"
#include "DDSSubscriber.hpp"

namespace lifecycle_manager {

class DDSMessageEndPoint  : public MessageEndPoint  {
  public:
    DDSMessageEndPoint();
    virtual ~DDSMessageEndPoint();

    bool Initialize();
    void Finalize();

    std::string GetTopic();
    std::string GetTypeName();
    GetTypeSupport();

    bool Write(Message* message);
    void Read(Message& message);
    bool Read(std::vector<Message>& messages);

  private:
    DDSPublisher* publisher_;
    DDSSubscriber* subscriber_;


};

} // namespace lifecycle_manager

#endif // LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_
