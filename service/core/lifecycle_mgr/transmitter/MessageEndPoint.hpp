
#ifndef LIFECYCLE_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

namespace lifecycle_manager {

class MessageEndPoint {
  public:
    MessageEndPoint(std::string topic, std::string type, std::shared_ptr<TypeSupport>  type_support);
    virtual ~MessageEndPoint();

    std::string GetTopic();
    std::string GetTypeName();
    std::shared_ptr<TypeSupport> GetTypeSupport();

    virtual void OnDataAvailable() = 0;
    virtual void* GetTopicBuffer() = 0;

  protected:
    std::string topic_;
    std::string type_name_;
    std::shared_ptr<TypeSupport> type_support_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_ENDPOINT_HPP_
