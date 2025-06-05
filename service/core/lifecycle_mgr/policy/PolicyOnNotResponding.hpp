#ifndef LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_
#define LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyOnNotResponding : PolicyBase {
  public:
    PolicyOnNotResponding();
    virtual ~PolicyOnNotResponding();

    std::string GetName();
    ServiceControlAction Evaluate();

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ON_NOT_RESPONDING_HPP_
