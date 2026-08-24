#ifndef LIFECYCLE_POLICY_ON_SUCCESS_HPP_
#define LIFECYCLE_POLICY_ON_SUCCESS_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyOnSuccess : public PolicyBase {
  public:
    PolicyOnSuccess(RecoveryPolicy policy, std::string name);
    ~PolicyOnSuccess() override= default;
    ServiceControlAction Evaluate(std::shared_ptr<Service> service) override;

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ON_SUCCESS_HPP_
