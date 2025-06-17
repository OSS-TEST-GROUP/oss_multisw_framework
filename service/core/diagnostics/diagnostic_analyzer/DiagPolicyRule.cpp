#include "DiagPolicyRule.hpp"

namespace diagnostic_manager {

DiagPolicyRule::DiagPolicyRule(DiagInfoStatus status, const std::string& message, std::vector<std::string> conditions, ConditionOperator condition_operator)
        : status(status), message(message), conditions(conditions), condition_operator(condition_operator) {}
DiagPolicyRule::~DiagPolicyRule() {}

} // namespace diagnostic_manager
