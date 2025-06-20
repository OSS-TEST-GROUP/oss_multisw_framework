#include "PolicyManager.hpp"

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <exception>
#include <filesystem>

namespace fs = std::filesystem;
namespace diagnostic_manager {

bool PolicyManager::Initialize() {
    policies_.clear();
    reference_msgs_.clear();

    return true;
}

bool PolicyManager::LoadAllPolicy(const std::string& path) {
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                LoadPolicy(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
        return false;
    }

    std::cout << "A total of " << policies_.size() << " policies were loaded." << std::endl;
    return true;
}

bool PolicyManager::LoadPolicy(const std::string& path) {
    YAML::Node root;
    try {
        // Load the YAML configuration file
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        // Failed to open or parse the YAML file
        return false;
    }

    std::vector<std::shared_ptr<DiagPolicy>> diag_polices_temp;

    // Parse diagnostic policies
    if (root["diagnosic_policies"] && root["diagnosic_policies"].IsSequence()) {
        YAML::Node policiesNode = root["diagnosic_policies"];
        for (const auto& policyNode : policiesNode) {
            // Required fields: code, name, rules, active
            if (!policyNode["code"] || !policyNode["name"]) {
                // Skip if essential fields are missing
                continue;
            }
            int code = policyNode["code"].as<int>();
            std::string name = policyNode["name"].as<std::string>();
            bool active = false;
            if (policyNode["active"]) {
                active = policyNode["active"].as<bool>();
            }

            // Parse rules if present
            std::vector<DiagPolicyRule> rules;

            if (policyNode["rules"] && policyNode["rules"].IsSequence()) {
                YAML::Node rulesNode = policyNode["rules"];
                for (const auto& ruleNode : rulesNode) {
                    // Each rule should have status and message (condition is optional list)
                    if (!ruleNode["status"] || !ruleNode["message"]) {
                        continue; // skip rules missing essential fields
                    }
                    std::string statusStr = ruleNode["status"].as<std::string>();
                    std::string messageStr = ruleNode["message"].as<std::string>();
                    std::string conditionOperatorStr = ruleNode["condition_operator"].as<std::string>();

                    // Convert status string to DiagInfoStatus enum
                    DiagInfoStatus status = DiagInfoStatus::UNKNOWN;
                    if (statusStr == "PASS") {
                        status = DiagInfoStatus::PASS;
                    } else if (statusStr == "FATAL") {
                        status = DiagInfoStatus::FATAL;
                    } else if (statusStr == "WARNING") {
                        status = DiagInfoStatus::WARNING;
                    } else {
                        status = DiagInfoStatus::UNKNOWN;
                    }

                    // Collect condition strings (if any)
                    std::vector<std::string> conditions;
                    if (ruleNode["condition"] && ruleNode["condition"].IsSequence()) {
                        for (const auto& condNode : ruleNode["condition"]) {
                            conditions.push_back(condNode.as<std::string>());
                        }
                    }

                    ConditionOperator condition_operator = ConditionOperator::AND;
                    if (conditionOperatorStr == "AND") {
                        condition_operator = ConditionOperator::AND;
                    } else if (conditionOperatorStr == "OR") {
                        condition_operator = ConditionOperator::OR;
                    }

                    // Create a new DiagPolicyRule object and add it to the policy
                    rules.emplace_back(status, messageStr, conditions, condition_operator);
                    // Assuming DiagPolicy has a method to add a rule (or accessible vector)
                }
            }
            
            // Create a new DiagPolicy object
            std::shared_ptr<DiagPolicy> policyObj = std::make_shared<DiagPolicy>(code, name, active, std::move(rules));

            // Add the completed policy to the list (only if active or regardless? likely include all)
            policies_.push_back(policyObj);
            diag_polices_temp.push_back(policyObj);
        }
    }

    // Parse reference message definitions
    if (root["refer_messages"] && root["refer_messages"].IsSequence()) {
        YAML::Node refMsgsNode = root["refer_messages"];
        for (const auto& refNode : refMsgsNode) {
            if (!refNode["name"] || !refNode["message"]) {
                continue;
            }
            std::string refName = refNode["name"].as<std::string>();
            std::string messageType = refNode["message"].as<std::string>();
            std::string protoStr;
            if (refNode["protocol"]) {
                protoStr = refNode["protocol"].as<std::string>();
            }

            // Convert protocol string to MessageProtocolType enum (default NOT_DEFINED)
            MessageProtocolType protoType = MessageProtocolType::NOT_DEFINED;
            if (!protoStr.empty()) {
                std::string protoLower = protoStr;
                std::transform(protoLower.begin(), protoLower.end(), protoLower.begin(), ::tolower);
                if (protoLower == "dds") {
                    protoType = MessageProtocolType::DDS;
                } else {
                    protoType = MessageProtocolType::NOT_DEFINED;
                }
            }

            // Create a ReferenceMessage object and add to the list
            std::shared_ptr<ReferenceMessage> refMsgObj = std::make_shared<ReferenceMessage>(refName, messageType, protoType);

            // Insert diag code to be referred
            for (auto& diag_policy : diag_polices_temp) {
                if (HasConditionUsingReferenceMessage(diag_policy, refName)) {
                    refMsgObj->SetDiagCode(diag_policy->GetDiagCode());
                }
            }

            reference_msgs_.push_back(refMsgObj);
        }
    }

    return true;
}

std::shared_ptr<DiagPolicy> PolicyManager::GetPolicy(int diag_code) {
    for (const auto& policy : policies_) {
        if (policy->GetDiagCode() == diag_code) {
            return policy;
        }
    }
    return nullptr;
}

std::shared_ptr<DiagPolicy> PolicyManager::GetPolicyByIndex(size_t idx) {
    if (idx < policies_.size()) {
        return policies_[idx];
    }
    return nullptr;
}

std::shared_ptr<DiagPolicy> PolicyManager::GetPolicyByName(const std::string& name) {
    for (const auto& policy : policies_) {
        if (policy->GetDiagName() == name) {
            return policy;
        }
    }
    return nullptr;
}

std::shared_ptr<ReferenceMessage> PolicyManager::GetReferenceMessage(const std::string& name) {
    for (const auto& refMsg : reference_msgs_) {
        if (refMsg->GetName() == name) {
            return refMsg;
        }
    }
    return nullptr;
}

std::shared_ptr<ReferenceMessage> PolicyManager::GetReferenceMessageByMessageName(const std::string& name) {
    for (const auto& refMsg : reference_msgs_) {
        if (refMsg->GetMessage() == name) {
            return refMsg;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<ReferenceMessage>> PolicyManager::GetAllReferenceMessage() {
    return reference_msgs_;
}

uint64_t PolicyManager::GetPolicyNum() {
    return static_cast<uint64_t>(policies_.size());
}

uint64_t PolicyManager::GetReferenceMessageNum() {
    return static_cast<uint64_t>(reference_msgs_.size());
}

bool PolicyManager::HasConditionUsingReferenceMessage(std::shared_ptr<DiagPolicy> policy, const std::string& message) {
    for (auto& diag_rule : policy->GetRules()) {
        for (auto& condition : diag_rule.conditions) {
            if (condition.find(message) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

} // namespace diagnostic_manager
