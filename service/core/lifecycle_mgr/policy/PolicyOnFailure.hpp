#ifndef LIFECYCLE_POLICY_ON_FAILURE_HPP_
#define LIFECYCLE_POLICY_ON_FAILURE_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyOnFailure : public PolicyBase {
  public:
    PolicyOnFailure(RecoveryPolicy policy, std::string name);
    virtual ~PolicyOnFailure();

    ServiceControlAction Evaluate(std::shared_ptr<Service> service) override;

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ON_FAILURE_HPP_
