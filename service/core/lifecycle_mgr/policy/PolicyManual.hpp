#ifndef LIFECYCLE_POLICY_MANUAL_HPP_
#define LIFECYCLE_POLICY_MANUAL_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyManual : public PolicyBase {
  public:
    PolicyManual(RecoveryPolicy policy, std::string name);
    ~PolicyManual() override= default;
    ServiceControlAction Evaluate(std::shared_ptr<Service> service) override;

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ALWAYS_HPP_
