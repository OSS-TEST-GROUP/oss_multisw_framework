#include "DiagPolicy.hpp"

namespace diagnostic_manager {

DiagPolicy::DiagPolicy(int32_t code, const std::string& name, bool active, const std::vector<DiagPolicyRule>& rules)
    : diag_code_(code), diag_name_(name), is_active_(active), rules_(rules) {}

int32_t DiagPolicy::GetDiagCode() const {
    return diag_code_;
}

std::string DiagPolicy::GetDiagName() const {
    return diag_name_;
}

std::vector<DiagPolicyRule> DiagPolicy::GetRules() const {
    return rules_;
}

bool DiagPolicy::IsActive() const {
    return is_active_;
}

} // namespace diagnostic_manager
