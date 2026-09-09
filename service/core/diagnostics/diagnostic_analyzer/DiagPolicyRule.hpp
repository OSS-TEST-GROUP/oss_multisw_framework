#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_RULE_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_RULE_HPP_

#include <iostream>
#include <vector>

#include "diagnostic/DiagInfo.hpp"

namespace diagnostic_manager {

enum class ConditionOperator {
    AND,
    OR
};

class DiagPolicyRule {
  public:
    DiagPolicyRule(DiagInfoStatus status, const std::string& message, const std::vector<std::string>& conditions, ConditionOperator condition_operator);
    virtual ~DiagPolicyRule();

    DiagInfoStatus status;
    std::string message;
    std::vector<std::string> conditions;
    ConditionOperator condition_operator;
};

} // namespace diagnostic_manager

#endif
