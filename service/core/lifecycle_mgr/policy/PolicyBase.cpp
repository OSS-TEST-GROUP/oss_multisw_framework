#include "PolicyBase.hpp"

namespace lifecycle_manager {

PolicyBase::PolicyBase(RecoveryPolicy policy, std::string name ) : policy_(policy), name_(name) {
}
PolicyBase::~PolicyBase() {}


RecoveryPolicy PolicyBase::GetPolicy(){
    return policy_;
}

std::string PolicyBase::GetName(){
    return name_;
}


} // namespace lifecycle_manager
