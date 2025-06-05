
#ifndef LIFECYCLE_MESSAGE_ENDPOINT_HPP_
#define LIFECYCLE_MESSAGE_ENDPOINT_HPP_

#include <string>
#include <set>
#include <memory>

namespace lifecycle_manager {

class MessageEndPoint {
  public:
    MessageEndPoint();
    virtual ~MessageEndPoint();

    std::string GetTopic();
    std::string GetTypeName();
    std::shared_ptr<TypeSupport> GetTypeSupport();

    void onDataAvailable();

  private:
    TypeSupport type_support_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MESSAGE_ENDPOINT_HPP_
