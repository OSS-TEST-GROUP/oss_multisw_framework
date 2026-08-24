#ifndef LIFECYCLE_POLICY_BASE_HPP_
#define LIFECYCLE_POLICY_BASE_HPP_

#include <string>
#include <memory>

#include "include/RecoveryPolicy.hpp"
#include "include/ServiceControlAction.hpp"
#include "service/Service.hpp"

namespace lifecycle_manager {

class PolicyBase {
  public:
    PolicyBase(RecoveryPolicy policy, std::string name);
    virtual ~PolicyBase() = default;
    RecoveryPolicy GetPolicy();
    std::string GetName();

    virtual ServiceControlAction Evaluate(std::shared_ptr<Service> service) = 0;

  protected:
    RecoveryPolicy policy_;
    std::string name_;

};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_BASE_HPP_
