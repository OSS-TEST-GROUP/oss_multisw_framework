#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_CONDITION_EVALUATOR_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_CONDITION_EVALUATOR_HPP_

#include <unordered_map>
#include <set>
#include <memory>
#include <string>
#include <mutex>

#include "DataManager.hpp"
#include "PolicyManager.hpp"

namespace diagnostic_manager {

enum class ConditionOperatorType {
    UNKNOWN,
    EQUAL,              // =
    NOT_EQUAL,          // !=
    EQUAL_ABOVE,        // >=
    EQUAL_LESS,         // <=
    ABOVE,              // >
    LESS,               // <
    IN,                 // in
    NOT_IN,             // not in
};

enum class ConditionOperandType {
    UNKNOWN,
    INTEGER,
    FLOATING,
    BOOLEAN,
    STRING,
};

class ConditionExpression {
  public:
    std::string lh;
    std::string rh;
    ConditionOperandType operand_type{ConditionOperandType::UNKNOWN};
    ConditionOperatorType operator_type{ConditionOperatorType::UNKNOWN};
};

class ConditionEvaluator {
  public:
    ConditionEvaluator(std::shared_ptr<DataManager> data_manager, std::shared_ptr<PolicyManager> policy_manager);
    virtual ~ConditionEvaluator();

    bool EvaluateCondition(const std::string& condition);

    ConditionExpression ParseConditionExpression(const std::string& condition);
    ConditionOperandType EvaluateOperandType(ConditionExpression& cond_expression);

    std::vector<std::string> Tokenize(const std::string& s, char delimiter);
    void ConcatReleatedToken(std::vector<std::string>& token_list);

    std::string ConvertOperatorTypeToString(ConditionOperatorType type);
    ConditionOperatorType ConvertStringToOperatorType(const std::string& type);
    ConditionOperandType ConvertTypeIndexToOperandType(std::type_index type);

    template<typename T>
    bool EvaluateOperandByOperator(const T& lh, const T& rh, ConditionOperatorType op);

    template<typename T>
    bool EvaluateOperandByOperatorWithList(const T& lh, const std::string& rh, ConditionOperatorType op);

    template<typename T>
    std::set<T> ParseArrayValue(const std::string& operand);

  private:
    bool IsConstantValue(const std::string& operand);
    bool IsArrayValue(const std::string& operand);
    ConditionOperandType EvaluateOperandTypeFromConstantValue(const std::string& operand);

    std::string ResolveMessageName(const std::string& name);

    template<typename T>
    bool FetchOrParseValue(const std::string& operand, T& out);

    std::shared_ptr<DataManager> data_manager_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unordered_map<std::string, ConditionExpression> condition_cache_;
};

}  // namespace diagnostic_manager

#endif // DIAGNOSTICS_DIAGNOSTIC_ANALYZER_CONDITION_EVALUATOR_HPP_
