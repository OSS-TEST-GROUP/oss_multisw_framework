#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_POLICY_MANAGER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_POLICY_MANAGER_HPP_

#include <string>
#include <vector>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "DiagPolicy.hpp"
#include "DiagPolicyRule.hpp"
#include "ReferenceMessage.hpp"

namespace diagnostic_manager {

class PolicyManager {
public:
    bool Initialize();

    bool LoadAllPolicy(const std::string& path);
    bool LoadPolicy(const std::string& filepath);

    std::shared_ptr<DiagPolicy> GetPolicy(int diag_code);
    std::shared_ptr<DiagPolicy> GetPolicyByIndex(size_t idx);
    std::shared_ptr<DiagPolicy> GetPolicyByName(const std::string& name);

    std::shared_ptr<ReferenceMessage> GetReferenceMessage(const std::string& name);
    std::shared_ptr<ReferenceMessage> GetReferenceMessageByMessageName(const std::string& name);
    std::vector<std::shared_ptr<ReferenceMessage>> GetAllReferenceMessage();

    uint64_t GetPolicyNum();
    uint64_t GetReferenceMessageNum();

private:
    bool HasConditionUsingReferenceMessage(std::shared_ptr<DiagPolicy> policy, const std::string& message);

    // List of diagnostic policies
    std::vector<std::shared_ptr<DiagPolicy>> policies_;

    // List of reference message definitions
    std::vector<std::shared_ptr<ReferenceMessage>> reference_msgs_;
};

} // namespace diagnostic_manager

#endif // DIAGNOSTICS_DIAGNOSTIC_ANALYZER_POLICY_MANAGER_HPP_
