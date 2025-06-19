#ifndef LIFECYCLE_POLICY_ALWAYS_HPP_
#define LIFECYCLE_POLICY_ALWAYS_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyAlways : public PolicyBase {
  public:
    PolicyAlways(RecoveryPolicy policy, std::string name);
    virtual ~PolicyAlways();

    ServiceControlAction Evaluate(std::shared_ptr<Service> service) override;

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ALWAYS_HPP_
