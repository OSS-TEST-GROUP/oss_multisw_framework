#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_HPP_

#include <string>
#include <vector>
#include "diagnostic_analyzer/DiagPolicyRule.hpp"

namespace diagnostic_manager {

class DiagPolicy {
public:
    DiagPolicy(int32_t code, const std::string& name, bool active, const std::vector<DiagPolicyRule>& rules);

    int32_t GetDiagCode() const;
    std::string GetDiagName() const;
    std::vector<DiagPolicyRule> GetRules() const;
    bool IsActive() const;

private:
    int32_t diag_code_;
    std::string diag_name_;
    bool is_active_;
    std::vector<DiagPolicyRule> rules_;
};

} // namespace diagnostic_manager

#endif // DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_HPP_
