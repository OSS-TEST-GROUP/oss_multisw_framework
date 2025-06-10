#ifndef LIFECYCLE_POLICY_MANUAL_HPP_
#define LIFECYCLE_POLICY_MANUAL_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyManual : PolicyBase {
  public:
    PolicyManual();
    virtual ~PolicyManual();

    std::string GetName();
    ServiceControlAction Evaluate();

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ALWAYS_HPP_
