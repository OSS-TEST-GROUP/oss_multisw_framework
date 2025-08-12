#include "ConditionEvaluator.hpp"

#include <sstream>
#include <regex>

#include "logger/Log.hpp"

namespace diagnostic_manager {

ConditionEvaluator::ConditionEvaluator(std::shared_ptr<DataManager> data_manager, std::shared_ptr<PolicyManager> policy_manager) :
    data_manager_(data_manager), policy_manager_(policy_manager) {}

ConditionEvaluator::~ConditionEvaluator() {}

bool ConditionEvaluator::EvaluateCondition(const std::string& condition) {
    ConditionExpression cond;

    // Load from cache if existing.
    if (condition_cache_.count(condition) > 0) {
        cond = condition_cache_[condition];
    } else {
        cond = ParseConditionExpression(condition);
        condition_cache_[condition] = cond;
    }

    if (cond.operator_type == ConditionOperatorType::UNKNOWN) {
        LOG_ERROR("Failed to parse a policy condition. condition=" << condition);
        return false;
    }

    if (cond.operand_type == ConditionOperandType::UNKNOWN) {
        auto operand_type = EvaluateOperandType(cond);
        if (operand_type == ConditionOperandType::UNKNOWN) {
            LOG_ERROR("Cannot evaluate the operand type. condition=" << condition);
            return false;
        }
        cond.operand_type = operand_type;
    }

    // Evaluate with list
    if (cond.operator_type == ConditionOperatorType::IN || cond.operator_type == ConditionOperatorType::NOT_IN) {
        switch (cond.operand_type) {
            case ConditionOperandType::INTEGER: {
                int64_t lh;
                if (FetchOrParseValue(cond.lh, lh)) {
                    return EvaluateOperandByOperatorWithList(lh, cond.rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::FLOATING: {
                double lh;
                if (FetchOrParseValue(cond.lh, lh)) {
                    return EvaluateOperandByOperatorWithList(lh, cond.rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::BOOLEAN: {
                bool lh;
                if (FetchOrParseValue(cond.lh, lh)) {
                    return EvaluateOperandByOperatorWithList(lh, cond.rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::STRING: {
                std::string lh;
                if (FetchOrParseValue(cond.lh, lh)) {
                    return EvaluateOperandByOperatorWithList(lh, cond.rh, cond.operator_type);
                }
            } break;
            default: {
                // Do nothing
            }
        }
    } else {
        switch (cond.operand_type) {
            case ConditionOperandType::INTEGER: {
                int64_t lh, rh;
                if (FetchOrParseValue(cond.lh, lh) && FetchOrParseValue(cond.rh, rh)) {
                    return EvaluateOperandByOperator(lh, rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::FLOATING: {
                double lh, rh;
                if (FetchOrParseValue(cond.lh, lh) && FetchOrParseValue(cond.rh, rh)) {
                    return EvaluateOperandByOperator(lh, rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::BOOLEAN: {
                bool lh, rh;
                if (FetchOrParseValue(cond.lh, lh) && FetchOrParseValue(cond.rh, rh)) {
                    return EvaluateOperandByOperator(lh, rh, cond.operator_type);
                }
            } break;
            case ConditionOperandType::STRING: {
                std::string lh, rh;
                if (FetchOrParseValue(cond.lh, lh) && FetchOrParseValue(cond.rh, rh)) {
                    return EvaluateOperandByOperator(lh, rh, cond.operator_type);
                }
            } break;
            default: {
                // Do nothing
            }
        }
    }

    return false;
}

ConditionExpression ConditionEvaluator::ParseConditionExpression(const std::string& condition) {
    ConditionExpression expression;

    static const std::regex pattern(
        R"(^\s*(.*?)\s+(not in|in|!=|=|>=|<=|>|<)\s+(\[.*\]|\".*?\"|[^ ]+)\s*$)",
        std::regex::icase
    );

    std::smatch match;
    if (!std::regex_match(condition, match, pattern) || match.size() != 4) {
        LOG_ERROR("Invalid condition syntax: " << condition);
        return expression;
    }

    expression.lh = match[1].str();
    std::string op_str = match[2].str();
    expression.operator_type = ConvertStringToOperatorType(op_str);
    expression.rh = match[3].str();

    return expression;
}

ConditionOperandType ConditionEvaluator::EvaluateOperandType(ConditionExpression& cond_expression) {
    std::set<ConditionOperandType> operand_types;

    if (!IsConstantValue(cond_expression.lh)) {
        VariantValue value;
        data_manager_->FetchValueAsVariant(ResolveMessageName(cond_expression.lh), value);
        operand_types.insert(ConvertTypeIndexToOperandType(value.type));
    } else {
        operand_types.insert(EvaluateOperandTypeFromConstantValue(cond_expression.lh));
    }

    if (!IsConstantValue(cond_expression.rh)) {
        VariantValue value;
        data_manager_->FetchValueAsVariant(ResolveMessageName(cond_expression.rh), value);
        operand_types.insert(ConvertTypeIndexToOperandType(value.type));
    } else {
        operand_types.insert(EvaluateOperandTypeFromConstantValue(cond_expression.rh));
    }

    if (operand_types.size() == 1) {
        return *operand_types.begin();
    }

    if (operand_types.count(ConditionOperandType::INTEGER) > 0 && operand_types.count(ConditionOperandType::FLOATING)) {
        return ConditionOperandType::FLOATING;
    }
    if (operand_types.count(ConditionOperandType::INTEGER) > 0 && operand_types.count(ConditionOperandType::BOOLEAN)) {
        return ConditionOperandType::BOOLEAN;
    }
    return ConditionOperandType::UNKNOWN;
}

std::vector<std::string> ConditionEvaluator::Tokenize(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (token == "") {
            continue;
        }
        tokens.push_back(token);
    }

    return tokens;
}

void ConditionEvaluator::ConcatReleatedToken(std::vector<std::string>& token_list) {
    static std::vector<std::string> related_token_list = {
        "not"
    };

    for (uint32_t idx = 0; idx < token_list.size(); ) {
        bool related = false;
        uint32_t related_token_idx = 0;
        for (uint32_t token_idx = 0; token_idx < related_token_list.size(); token_idx++) {
            if (related_token_list[token_idx] == token_list[idx]) {
                related = true;
                related_token_idx = token_idx;
                break;
            }
        }
        if (related) {
            if (idx + 1 >= token_list.size()) {
                LOG_ERROR("Invalid condition. It ends with '" << related_token_list[idx] << "'.");
                return;
            }
            std::string new_token = related_token_list[related_token_idx] + " " + token_list[idx+1];
            token_list[idx] = new_token;
            token_list.erase(token_list.begin() + idx + 1);
        } else {
            idx++;
        }
    }
}

static std::map<ConditionOperatorType, std::string> operator_convert_map = {
    { ConditionOperatorType::EQUAL, "=" },
    { ConditionOperatorType::NOT_EQUAL, "!=" },
    { ConditionOperatorType::EQUAL_ABOVE, ">=" },
    { ConditionOperatorType::EQUAL_LESS, "<=" },
    { ConditionOperatorType::ABOVE, ">" },
    { ConditionOperatorType::LESS, "<" },
    { ConditionOperatorType::IN, "in" },
    { ConditionOperatorType::NOT_IN, "not in" }
};

std::string ConditionEvaluator::ConvertOperatorTypeToString(ConditionOperatorType type) {
    if (operator_convert_map.count(type) > 0) {
        return operator_convert_map[type];
    }
    return "";
}

ConditionOperatorType ConditionEvaluator::ConvertStringToOperatorType(const std::string& type) {
    for (auto& iter : operator_convert_map) {
        if (iter.second == type) {
            return iter.first;
        }
    }
    return ConditionOperatorType::UNKNOWN;
}

ConditionOperandType ConditionEvaluator::ConvertTypeIndexToOperandType(std::type_index type) {
    // Integer types
    if (type == typeid(char) ||
        type == typeid(wchar_t) ||
        type == typeid(int8_t) ||
        type == typeid(int16_t) ||
        type == typeid(int32_t) ||
        type == typeid(int64_t) ||
        type == typeid(uint8_t) ||
        type == typeid(uint16_t) ||
        type == typeid(uint32_t) ||
        type == typeid(uint64_t)) {
        return ConditionOperandType::INTEGER;
    }

    // Floating-point types
    if (type == typeid(float) ||
        type == typeid(double)) {
        return ConditionOperandType::FLOATING;
    }

    // Boolean type
    if (type == typeid(bool)) {
        return ConditionOperandType::BOOLEAN;
    }

    // String types
    if (type == typeid(std::string) ||
        type == typeid(std::wstring)) {
        return ConditionOperandType::STRING;
    }

    // Fallback
    return ConditionOperandType::UNKNOWN;
}

bool ConditionEvaluator::IsConstantValue(const std::string& operand) {
    static const std::regex value_pattern(
        R"regex(^\s*(true|false|\d+(\.\d+)?|\"[^\"]*\")\s*$|^\s*\[\s*(?:"[^"]*"|true|false|-?\d+\.\d+|-?\d+)(\s*,\s*(?:"[^"]*"|true|false|-?\d+\.\d+|-?\d+))*\s*\]\s*$)regex",
        std::regex::icase);

    return std::regex_match(operand, value_pattern);
}

bool ConditionEvaluator::IsArrayValue(const std::string& operand) {
    std::regex is_array_pattern(R"(^\s*\[.*\]\s*$)", std::regex::icase);

    return std::regex_match(operand, is_array_pattern);
}

ConditionOperandType ConditionEvaluator::EvaluateOperandTypeFromConstantValue(const std::string& operand) {
    static const std::regex boolean_pattern(R"(^\s*(true|false)\s*$)", std::regex::icase);
    static const std::regex boolean_array_pattern(R"(^\s*\[\s*(true|false)\s*(\s*,\s*(true|false)\s*)*\]\s*$)", std::regex::icase);
    static const std::regex string_pattern(R"(^\s*\".*\"\s*$)");
    static const std::regex string_array_pattern(R"(^\s*\[\s*\"[^\"]*\"\s*(\s*,\s*\"[^\"]*\"\s*)*\]\s*$)");
    static const std::regex integer_pattern(R"(^\s*-?\d+\s*$)");
    static const std::regex float_pattern(R"(^\s*-?\d*\.\d+\s*$)");
    static const std::regex float_array_pattern(R"(^\s*\[\s*(-?\d*\.\d+)(\s*,\s*-?\d*\.\d+)*\s*\]\s*$)");
    static const std::regex integer_array_pattern(R"(^\s*\[\s*(-?\d+)(\s*,\s*-?\d+)*\s*\]\s*$)");

    if (std::regex_match(operand, boolean_pattern)) {
        return ConditionOperandType::BOOLEAN;
    }

    if (std::regex_match(operand, boolean_array_pattern)) {
        return ConditionOperandType::BOOLEAN;
    }

    if (std::regex_match(operand, string_pattern)) {
        return ConditionOperandType::STRING;
    }

    if (std::regex_match(operand, string_array_pattern)) {
        return ConditionOperandType::STRING;
    }

    if (std::regex_match(operand, integer_pattern)) {
        return ConditionOperandType::INTEGER;
    }

    if (std::regex_match(operand, float_pattern)) {
        return ConditionOperandType::FLOATING;
    }

    if (std::regex_match(operand, float_array_pattern)) {
        return ConditionOperandType::FLOATING;
    }

    if (std::regex_match(operand, integer_array_pattern)) {
        return ConditionOperandType::INTEGER;
    }

    return ConditionOperandType::UNKNOWN;
}

template<typename T>
bool ConditionEvaluator::EvaluateOperandByOperator(T lh, T rh, ConditionOperatorType op) {
    if (op == ConditionOperatorType::EQUAL) {
        return lh == rh;
    } else if (op == ConditionOperatorType::NOT_EQUAL) {
        return lh != rh;
    } else if (op == ConditionOperatorType::EQUAL_ABOVE) {
        if constexpr (std::is_arithmetic_v<T>) {
            return lh >= rh;
        } else {
            LOG_ERROR("Failed to evaluate condition. The operator 'EQUAL_ABOVE' is not supported with this type.");
            return false;
        }
    } else if (op == ConditionOperatorType::EQUAL_LESS) {
        if constexpr (std::is_arithmetic_v<T>) {
            return lh <= rh;
        } else {
            LOG_ERROR("Failed to evaluate condition. The operator 'EQUAL_LESS' is not supported with this type.");
            return false;
        }
    } else if (op == ConditionOperatorType::ABOVE) {
        if constexpr (std::is_arithmetic_v<T>) {
            return lh > rh;
        } else {
            LOG_ERROR("Failed to evaluate condition. The operator 'ABOVE' is not supported with this type.");
            return false;
        }
    } else if (op == ConditionOperatorType::LESS) {
        if constexpr (std::is_arithmetic_v<T>) {
            return lh < rh;
        } else {
            LOG_ERROR("Failed to evaluate condition. The operator 'LESS' is not supported with this type.");
            return false;
        }
    }

    LOG_ERROR("Failed to evaluate condition. The operator " << (uint32_t)op << " is not supported on this type ");
    return false;
}

template<typename T>
bool ConditionEvaluator::EvaluateOperandByOperatorWithList(T lh, const std::string& rh, ConditionOperatorType op) {
    if (!IsArrayValue(rh)) {
        LOG_ERROR("The operator 'IN' and 'NOT_IN' require a list value.");
        return false;
    }

    std::set<T> list = ParseArrayValue<T>(rh);

    if (op == ConditionOperatorType::IN) {
        return list.count(lh) > 0;
    } else if (op == ConditionOperatorType::NOT_IN) {
        return list.count(lh) == 0;
    }

    LOG_ERROR("Failed to evaluate condition. The operator " << (uint32_t)op << " is not supported on this type ");
    return false;
}

std::string ConditionEvaluator::ResolveMessageName(const std::string& name) {
    size_t msg_name_pos = name.find('.');
    if (msg_name_pos == std::string::npos) {
        return name;
    }

    auto ref_message = policy_manager_->GetReferenceMessage(name.substr(0, msg_name_pos));
    if (!ref_message) {
        return name;
    }

    return ref_message->GetMessage() + name.substr(msg_name_pos);
}

template<typename T>
bool ConditionEvaluator::FetchOrParseValue(const std::string& operand, T& out) {
    std::string trimmed = operand;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), trimmed.end());

    if (trimmed.empty()) {
        LOG_ERROR("Empty string cannot be parsed.");
        return false;
    }

    if (IsConstantValue(trimmed)) {
        if constexpr (std::is_same_v<T, std::string>) {
            // If the string is quoted (e.g., "hello"), strip quotes
            if (trimmed.front() == '"' && trimmed.back() == '"' && trimmed.length() >= 2) {
                out = trimmed.substr(1, trimmed.length() - 2);
                return true;
            }
            out = trimmed;
            return true;
        } else if constexpr (std::is_same_v<T, bool>) {
            std::string lowered;
            std::transform(trimmed.begin(), trimmed.end(), std::back_inserter(lowered), ::tolower);
            if (lowered == "true") { out = true; return true; }
            if (lowered == "false") { out = false; return true; }
            LOG_ERROR("Invalid boolean string: " << trimmed);
            return false;
        } else {
            std::istringstream iss(trimmed);
            T value;
            iss >> value;
            if (iss.fail() || !iss.eof()) {
                LOG_ERROR("Failed to parse value from string: " << trimmed);
                return false;
            }
            out = value;
            return true;
        }
    } else {
        return data_manager_->FetchValue(ResolveMessageName(trimmed), out);
    }
}

template<typename T>
std::set<T> ConditionEvaluator::ParseArrayValue(const std::string& operand) {
    std::set<T> result;

    std::string trimmed = operand;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), [](unsigned char c) {
        return std::isspace(c);
    }), trimmed.end());

    if (trimmed.size() < 2 || trimmed.front() != '[' || trimmed.back() != ']') {
        LOG_ERROR("Invalid array format: " << operand);
        return result;
    }

    trimmed = trimmed.substr(1, trimmed.size() - 2); // remove '[' and ']'

    if (trimmed.empty()) return result;

    std::stringstream ss(trimmed);
    std::string token;

    while (std::getline(ss, token, ',')) {
        if constexpr (std::is_same_v<T, std::string>) {
            // Must be quoted
            if (token.size() < 2 || token.front() != '"' || token.back() != '"') {
                LOG_ERROR("String element must be quoted: " << token);
                result.clear();
                return result;
            }
            result.insert(token.substr(1, token.size() - 2)); // strip quotes
        } else if constexpr (std::is_same_v<T, bool>) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            if (token == "true") {
                result.insert(true);
            } else if (token == "false") {
                result.insert(false);
            } else {
                LOG_ERROR("Invalid boolean value: " << token);
                result.clear();
                return result;
            }
        } else {
            std::stringstream val_stream(token);
            T value;
            val_stream >> value;
            if (val_stream.fail()) {
                LOG_ERROR("Cannot convert to target type: " << token);
                result.clear();
                return result;
            }
            result.insert(value);
        }
    }

    return result;
}

}  // namespace diagnostic_manager
