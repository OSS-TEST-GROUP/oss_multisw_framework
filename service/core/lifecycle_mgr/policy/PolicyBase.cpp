#include "PolicyBase.hpp"

#include <utility>

namespace lifecycle_manager {

PolicyBase::PolicyBase(RecoveryPolicy policy, std::string name) : policy_(policy), name_(std::move(name)) {
}


RecoveryPolicy PolicyBase::GetPolicy(){
    return policy_;
}

std::string PolicyBase::GetName(){
    return name_;
}


} // namespace lifecycle_manager
