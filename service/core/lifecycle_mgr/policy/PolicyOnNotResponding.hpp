#ifndef LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_
#define LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyOnNotResponding : public PolicyBase {
  public:
    PolicyOnNotResponding(RecoveryPolicy policy, std::string name);
    virtual ~PolicyOnNotResponding();

    ServiceControlAction Evaluate(std::shared_ptr<Service> service) override;

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_
