#include "DiagPolicyRule.hpp"

namespace diagnostic_manager {

DiagPolicyRule::DiagPolicyRule(DiagInfoStatus status, const std::string& message, std::vector<std::string> conditions)
        : status(status), message(message), conditions(conditions) {}
DiagPolicyRule::~DiagPolicyRule() {}

} // namespace diagnostic_manager
