#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <regex>

#include "DiagnosticAnalyzer.hpp"
#include "request_msg/AnalyzeRequestMessage.hpp"
#include "request_msg/DiagMessage.hpp"

using namespace std::placeholders;

namespace diagnostic_manager {

const std::string DIAG_CONFIG_PATH = "config";

DiagnosticAnalyzer::DiagnosticAnalyzer(std::shared_ptr<ServiceExecutor> service_executor, std::shared_ptr<MessageTransmitter> msg_transmitter)
        : service_executor_(service_executor), msg_transmitter_(msg_transmitter) {}

DiagnosticAnalyzer::~DiagnosticAnalyzer() {}

bool DiagnosticAnalyzer::Initialize() {
    if (initialized_) {
        return true;
    }

    data_manager_ = std::make_shared<DataManager>();

    policy_manager_ = std::make_shared<PolicyManager>();
    if (!policy_manager_->Initialize()) {
        std::cerr << "Failed to initialize the PolicyManager." << std::endl;
        return false;
    }
    if (!policy_manager_->LoadAllPolicy(DIAG_CONFIG_PATH)) {
        std::cerr << "Failed to load diagnostic policies." << std::endl;
        return false;
    }

    condition_evaluator_ = std::make_shared<ConditionEvaluator>(data_manager_, policy_manager_);

    subscriber_id_ = msg_transmitter_->AddSubscriber("DiagnosticAnalyzer", std::bind(&DiagnosticAnalyzer::HandleReceiveDiagMessage, this, _1), []() {
        return std::make_shared<DiagMessage>();
    });
    if (subscriber_id_ < 0) {
        std::cerr << "Failed to register to the MessageTransmitter." << std::endl;
        return false;
    }

    service_executor_->AddTask("DiagnosticAnalyzerRequest", std::bind(&DiagnosticAnalyzer::HandleAnalyzeRequest, this, _1), AnalyzeRequestMessage());

    SubscribeReferenceMessages();

    initialized_ = true;
    return true;
}

void DiagnosticAnalyzer::Finalize() {
    if (!initialized_) {
        return;
    }
    initialized_ = false;

    msg_transmitter_->RemoveSubscriber(subscriber_id_);
}

std::vector<std::shared_ptr<DiagInfo>> DiagnosticAnalyzer::AnalyzeAll() {
    std::vector<std::shared_ptr<DiagInfo>> result;

    auto police_list = GetDiagPoliciesToAnalyze();

    for (const auto& policy : police_list) {
        if (!policy->IsActive()) {
            continue;
        }
        auto analysis_results = Analyze(policy);
        result.insert(result.end(), analysis_results.begin(), analysis_results.end());
    }

    std::cout << "--- Analyze Result ---" << std::endl;
    for (auto iter : result) {
        std::cout << "DiagInfo[" << iter->GetDiagCode() << "] " << iter->GetDiagName() << " status=" << (uint32_t)iter->GetStatus() << " message=" << iter->GetMessage() << std::endl;
    }
    if (result.size() == 0) {
        std::cout << "No analyzation info." << std::endl;
    }

    return result;
}

std::vector<std::shared_ptr<DiagInfo>> DiagnosticAnalyzer::Analyze(std::shared_ptr<DiagPolicy> diag_policy) {
    std::vector<std::shared_ptr<DiagInfo>> diag_info;

    for (auto& policy_rule : diag_policy->GetRules()) {
        bool evaluation_result = EvaluatePolicyRuleConditions(policy_rule.conditions, policy_rule.condition_operator);
        if (evaluation_result) {
            diag_info.push_back(std::make_shared<DiagInfo>(diag_policy->GetDiagCode(), diag_policy->GetDiagName(), policy_rule.status, policy_rule.message));
            break;
        }
    }

    return diag_info;
}

void DiagnosticAnalyzer::HandleReceiveDiagMessage(std::shared_ptr<RequestBase> req_msg) {
    auto diag_msg = std::dynamic_pointer_cast<DiagMessage>(req_msg);
    if (!diag_msg) return;
    auto diag_data = diag_msg->GetMessage()->GetData();
    if (!diag_data) return;

    data_manager_->UpdateData(diag_data);

    service_executor_->RequestTask(std::make_shared<AnalyzeRequestMessage>());
}

void DiagnosticAnalyzer::HandleAnalyzeRequest(std::shared_ptr<RequestBase> req_msg) {
    // TODO - To be improved not to analyze too frequently.

    AnalyzeAll();
}

bool DiagnosticAnalyzer::EvaluatePolicyRuleConditions(const std::vector<std::string>& conditions, ConditionOperator condition_operator) {
    bool or_success = false;
    for (auto& condition : conditions) {
        bool result = EvaluatePolicyRuleCondition(condition);

        if (result == true) {
            or_success = true;
        } else if (result == false && condition_operator == ConditionOperator::AND) {
            return false;
        }
    }

    return or_success;
}

bool DiagnosticAnalyzer::EvaluatePolicyRuleCondition(const std::string& condition) {
    return condition_evaluator_->EvaluateCondition(condition);
}

void DiagnosticAnalyzer::SubscribeReferenceMessages() {
    auto ref_msgs = policy_manager_->GetAllReferenceMessage();

    for (auto& msg : ref_msgs) {
        msg_transmitter_->SubscribeMessage(subscriber_id_, msg->GetMessage(), msg->GetProtocol());
    }
}

std::vector<std::shared_ptr<DiagPolicy>> DiagnosticAnalyzer::GetDiagPoliciesToAnalyze() {
    std::vector<std::shared_ptr<DiagPolicy>> policies;
    std::set<int32_t> diag_codes;

    auto updated_data_list = data_manager_->GetAllUpdatedData();
    for (auto& updated_data : updated_data_list) {
        auto ref_msg = policy_manager_->GetReferenceMessageByMessageName(updated_data->GetName());
        if (!ref_msg) {
            continue;
        }
        auto ref_msg_diag_codes = ref_msg->GetAllDiagCode();
        diag_codes.insert(ref_msg_diag_codes.begin(), ref_msg_diag_codes.end());
    }

    for (auto diag_code : diag_codes) {
        policies.push_back(policy_manager_->GetPolicy(diag_code));
    }

    return policies;
}

}  // namespace diagnostic_manager
