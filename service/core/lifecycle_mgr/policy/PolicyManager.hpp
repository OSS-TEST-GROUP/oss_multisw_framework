#ifndef LIFECYCLE_POLICY_MANAGER_HPP_
#define LIFECYCLE_POLICY_MANAGER_HPP_

#include <string>
#include <map>
#include <memory>

#include "PolicyBase.hpp"

namespace lifecycle_manager {

class PolicyManager {
  public:
    PolicyManager();
    virtual ~PolicyManager();

    bool Start();
    void Stop();
    
    std::shared_ptr<PolicyBase> GetPolicy(std::string name);
    ServiceControlAction Evaluate(std::string service);

  private:
    std::map<std::string, std::shared_ptr<PolicyBase>> policies_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_POLICY_MANAGER_HPP_
