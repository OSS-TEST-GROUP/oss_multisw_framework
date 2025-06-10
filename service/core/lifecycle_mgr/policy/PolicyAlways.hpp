#ifndef LIFECYCLE_POLICY_ALWAYS_HPP_
#define LIFECYCLE_POLICY_ALWAYS_HPP_

#include <string>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyAlways : PolicyBase {
  public:
    PolicyAlways();
    virtual ~PolicyAlways();

    std::string GetName();
    ServiceControlAction Evaluate();

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_ALWAYS_HPP_
