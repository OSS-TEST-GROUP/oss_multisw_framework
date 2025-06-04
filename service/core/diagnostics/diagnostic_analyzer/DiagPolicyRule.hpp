#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_RULE_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_POLICY_RULE_HPP_

#include <iostream>
#include <set>

#include "diagnostic/DiagInfo.hpp"

namespace diagnostic_manager {

class DiagPolicyRule {
  public:
    DiagPolicyRule(DiagInfoStatus status, const std::string& message, std::vector<std::string> conditions);
    virtual ~DiagPolicyRule();

    DiagInfoStatus status;
    std::string message;
    std::vector<std::string> conditions;
};

} // namespace diagnostic_manager

#endif
