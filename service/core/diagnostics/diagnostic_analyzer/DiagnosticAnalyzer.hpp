#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAGNOSTIC_ANALYZER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAGNOSTIC_ANALYZER_HPP_

#include <map>
#include <memory>
#include <string>
#include <mutex>

#include "PolicyManager.hpp"
#include "DataManager.hpp"
#include "ConditionEvaluator.hpp"
#include "request_msg/DiagMessage.hpp"
#include "transmitter/MessageTransmitter.hpp"

namespace diagnostic_manager {

class DiagnosticAnalyzer {
public:
    DiagnosticAnalyzer(std::shared_ptr<MessageTransmitter> msg_transmitter);
    virtual ~DiagnosticAnalyzer();

    bool Initialize();
    void Finalize();

    std::vector<std::shared_ptr<DiagInfo>> AnalyzeAll();
    std::vector<std::shared_ptr<DiagInfo>> Analyze(std::shared_ptr<DiagPolicy> diag_policy);

    void HandleReceiveDiagMessage(std::shared_ptr<RequestBase> req_msg);
    void HandleAnalyzeRequest(std::shared_ptr<RequestBase> req_msg);

    bool EvaluatePolicyRuleConditions(const std::vector<std::string>& conditions, ConditionOperator condition_operator);
    bool EvaluatePolicyRuleCondition(const std::string& condition);

private:
    void SubscribeReferenceMessages();
    std::vector<std::shared_ptr<DiagPolicy>> GetDiagPoliciesToAnalyze();

    bool initialized_{false};

    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<DataManager> data_manager_;
    std::shared_ptr<ConditionEvaluator> condition_evaluator_;

    std::shared_ptr<MessageTransmitter> msg_transmitter_;
    int64_t subscriber_id_{-1};
};

}  // namespace diagnostic_manager

#endif // DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAGNOSTIC_ANALYZER_HPP_
