#ifndef LIFECYCLE_POLICY_BASE_HPP_
#define LIFECYCLE_POLICY_BASE_HPP_

#include <string>

#include "include/ServiceControlAction.hpp"

namespace lifecycle_manager {

class PolicyBase {
  public:
    PolicyBase();
    virtual ~PolicyBase();

    std::string GetName();
    ServiceControlAction Evaluate();

  private:
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_BASE_HPP_
