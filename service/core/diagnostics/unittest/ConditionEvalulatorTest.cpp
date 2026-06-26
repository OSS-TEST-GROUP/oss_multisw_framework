#include <gtest/gtest.h>

#include "diagnostic_analyzer/ConditionEvaluator.cpp"
#include "diagnostic_analyzer/DataManager.cpp"
#include "diagnostic_analyzer/PolicyManager.cpp"
#include "diagnostic_analyzer/ReferenceMessage.cpp"
#include "diagnostic_analyzer/DiagPolicy.cpp"
#include "diagnostic_analyzer/DiagPolicyRule.cpp"
#include "diagnostic/VariantData.cpp"

#include "mock/MockLogger.hpp"

using namespace diagnostic_manager;

class ConditionEvaluatorTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        data_manager = std::make_shared<DataManager>();
        policy_manager = std::make_shared<PolicyManager>();
        ins = new ConditionEvaluator(data_manager, policy_manager);
        Mock_Logger = new MockLogger();
    }

    virtual void TearDown(){
        delete ins;
        data_manager.reset();
        policy_manager.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    template<typename ValueType>
    void AddTestValue(const std::string& name, const std::string& key, ValueType value) {
        auto data = std::make_shared<diagnostic_manager::VariantData>();
        data->SetName(name);
        data->SetValue(key, value);
        data_manager->UpdateData(data);
    }

    void AddReferenceMessage(const std::string& name, const std::string& msg, MessageProtocolType protocol) {
        auto ref_msg = std::make_shared<ReferenceMessage>(name, msg, protocol);
        policy_manager->reference_msgs_.push_back(ref_msg);
    }

    std::shared_ptr<DataManager> data_manager;
    std::shared_ptr<PolicyManager> policy_manager;
    ConditionEvaluator* ins;
};

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_Int_IN) {
    EXPECT_TRUE(ins->EvaluateCondition("5 in [1, 2, 5, 7]"));
    EXPECT_FALSE(ins->EvaluateCondition("3 in [4, 5, 6]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_String_NOT_IN) {
    EXPECT_TRUE(ins->EvaluateCondition("\"foo\" not in [\"bar\", \"baz\"]"));
    EXPECT_FALSE(ins->EvaluateCondition("\"bar\" not in [\"bar\", \"baz\"]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_Bool_IN) {
    EXPECT_TRUE(ins->EvaluateCondition("true in [true, false]"));
    EXPECT_FALSE(ins->EvaluateCondition("false in [true]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Int_IN) {
    AddTestValue("SensorMessage", "value", 10);
    AddReferenceMessage("Sensor", "SensorMessage", MessageProtocolType::DDS);
    EXPECT_TRUE(ins->EvaluateCondition("Sensor.value in [8, 9, 10, 11]"));
    EXPECT_FALSE(ins->EvaluateCondition("Sensor.value in [1, 2, 3]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Bool_NOT_IN) {
    AddTestValue("ConfigMessage", "enabled", true);
    AddReferenceMessage("Config", "ConfigMessage", MessageProtocolType::DDS);
    EXPECT_FALSE(ins->EvaluateCondition("Config.enabled not in [true]"));
    EXPECT_TRUE(ins->EvaluateCondition("Config.enabled not in [false]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_String_IN) {
    AddTestValue("MetaMessage", "type", std::string("sensor"));
    AddReferenceMessage("Meta", "MetaMessage", MessageProtocolType::DDS);
    EXPECT_TRUE(ins->EvaluateCondition("Meta.type in [\"sensor\", \"actuator\"]"));
    EXPECT_FALSE(ins->EvaluateCondition("Meta.type in [\"camera\"]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_EmptyListReturnsFalse) {
    AddTestValue("Test", "val", 42);
    EXPECT_FALSE(ins->EvaluateCondition("Test.val in []"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_Int_Equality) {
    EXPECT_TRUE(ins->EvaluateCondition("5 = 5"));
    EXPECT_FALSE(ins->EvaluateCondition("5 = 6"));
    EXPECT_TRUE(ins->EvaluateCondition("5 != 6"));
    EXPECT_FALSE(ins->EvaluateCondition("5 != 5"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_Double_GreaterLess) {
    EXPECT_TRUE(ins->EvaluateCondition("3.14 > 2.5"));
    EXPECT_FALSE(ins->EvaluateCondition("3.14 < 2.5"));
    EXPECT_TRUE(ins->EvaluateCondition("3.14 >= 3.14"));
    EXPECT_FALSE(ins->EvaluateCondition("3.14 <= 2.0"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Constant_String_Equality) {
    EXPECT_TRUE(ins->EvaluateCondition("\"abc\" = \"abc\""));
    EXPECT_FALSE(ins->EvaluateCondition("\"abc\" = \"def\""));
    EXPECT_TRUE(ins->EvaluateCondition("\"abc\" != \"xyz\""));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Int_Comparison) {
    AddTestValue("Sensor", "temp", 25);
    EXPECT_TRUE(ins->EvaluateCondition("Sensor.temp >= 20"));
    EXPECT_FALSE(ins->EvaluateCondition("Sensor.temp < 10"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Double_Comparison) {
    AddTestValue("Speed", "value", 12.5);
    EXPECT_TRUE(ins->EvaluateCondition("Speed.value <= 15.0"));
    EXPECT_FALSE(ins->EvaluateCondition("Speed.value > 20.0"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Bool_Equality) {
    AddTestValue("Feature", "enabled", true);
    EXPECT_TRUE(ins->EvaluateCondition("Feature.enabled = true"));
    EXPECT_FALSE(ins->EvaluateCondition("Feature.enabled = false"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_String_Equality) {
    AddTestValue("Config", "mode", std::string("auto"));
    EXPECT_TRUE(ins->EvaluateCondition("Config.mode = \"auto\""));
    EXPECT_FALSE(ins->EvaluateCondition("Config.mode != \"auto\""));
}

TEST_F(ConditionEvaluatorTest, ParseConditionOperand_TC001) {
    const std::string condition = "LifecycleDiagReport.service_list[].service_state not in [1,2]";
    ConditionExpression expression = ins->ParseConditionExpression(condition);
    EXPECT_EQ("LifecycleDiagReport.service_list[].service_state", expression.lh);
    EXPECT_EQ("[1,2]", expression.rh);
    EXPECT_EQ(ConditionOperatorType::NOT_IN, expression.operator_type);
    EXPECT_EQ(ConditionOperandType::UNKNOWN, expression.operand_type);

    const std::string condition2 = "LifecycleDiagReport.initialized = false";
    ConditionExpression expression2 = ins->ParseConditionExpression(condition2);
    EXPECT_EQ("LifecycleDiagReport.initialized", expression2.lh);
    EXPECT_EQ("false", expression2.rh);
    EXPECT_EQ(ConditionOperatorType::EQUAL, expression2.operator_type);
    EXPECT_EQ(ConditionOperandType::UNKNOWN, expression2.operand_type);

    const std::string condition3 = "ADSWManagerDiagReport.isConnected  != true";
    ConditionExpression expression3 = ins->ParseConditionExpression(condition3);
    EXPECT_EQ("ADSWManagerDiagReport.isConnected", expression3.lh);
    EXPECT_EQ("true", expression3.rh);
    EXPECT_EQ(ConditionOperatorType::NOT_EQUAL, expression3.operator_type);
    EXPECT_EQ(ConditionOperandType::UNKNOWN, expression3.operand_type);

    const std::string condition4 = "LifecycleDiagReport.num_services >= 10";
    ConditionExpression expression4 = ins->ParseConditionExpression(condition4);
    EXPECT_EQ("LifecycleDiagReport.num_services", expression4.lh);
    EXPECT_EQ("10", expression4.rh);
    EXPECT_EQ(ConditionOperatorType::EQUAL_ABOVE, expression4.operator_type);
    EXPECT_EQ(ConditionOperandType::UNKNOWN, expression4.operand_type);

    const std::string condition5 = "LifecycleDiagReport.services[0].name = \"PerceptionNode\"";
    ConditionExpression expression5 = ins->ParseConditionExpression(condition5);
    EXPECT_EQ("LifecycleDiagReport.services[0].name", expression5.lh);
    EXPECT_EQ("\"PerceptionNode\"", expression5.rh);
    EXPECT_EQ(ConditionOperatorType::EQUAL, expression5.operator_type);
    EXPECT_EQ(ConditionOperandType::UNKNOWN, expression5.operand_type);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_Constant_IntegerVsFloat) {
    ConditionExpression expr;
    expr.lh = "10";
    expr.rh = "1.5";
    expr.operator_type = ConditionOperatorType::LESS;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::FLOATING);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_Constant_IntegerArrayVsInt) {
    ConditionExpression expr;
    expr.lh = "[1, 2, 3]";
    expr.rh = "2";
    expr.operator_type = ConditionOperatorType::IN;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::INTEGER);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_Constant_BooleanArray) {
    ConditionExpression expr;
    expr.lh = "true";
    expr.rh = "[true,true,false,false]";
    expr.operator_type = ConditionOperatorType::NOT_IN;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::BOOLEAN);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_Constant_StringComparison) {
    ConditionExpression expr;
    expr.lh = "\"A\"";
    expr.rh = "\"B\"";
    expr.operator_type = ConditionOperatorType::EQUAL;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::STRING);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_Variables_IntegerAndBoolean) {
    auto vdata = std::make_shared<VariantData>();
    vdata->SetName("Test");
    vdata->SetValue("num", static_cast<int32_t>(42));
    vdata->SetValue("flag", true);
    data_manager->UpdateData(vdata);

    ConditionExpression expr;
    expr.lh = "Test.num";
    expr.rh = "Test.flag";
    expr.operator_type = ConditionOperatorType::NOT_EQUAL;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::BOOLEAN);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_MixedConstantAndVariable_FloatAndInt) {
    auto vdata = std::make_shared<VariantData>();
    vdata->SetName("Numbers");
    vdata->SetValue("value", 3.14f);
    data_manager->UpdateData(vdata);

    ConditionExpression expr;
    expr.lh = "Numbers.value";
    expr.rh = "1";
    expr.operator_type = ConditionOperatorType::ABOVE;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::FLOATING);
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandType_UnknownDueToInvalidPath) {
    // EvaluateOperandType intentionally falls back to the *known* operand type
    // when only one side is UNKNOWN (see ConditionEvaluator.cpp). To exercise
    // the genuinely-unknown result, both operands must resolve to UNKNOWN —
    // use two invalid (non-constant) paths.
    ConditionExpression expr;
    expr.lh = "NonExistent.path";
    expr.rh = "AnotherNonExistent.path";
    expr.operator_type = ConditionOperatorType::EQUAL;

    ConditionOperandType type = ins->EvaluateOperandType(expr);
    EXPECT_EQ(type, ConditionOperandType::UNKNOWN);
}

TEST_F(ConditionEvaluatorTest, IsConstantValue_TC001) {
    EXPECT_TRUE(ins->IsConstantValue("[1,2]"));                // from: not in [1,2]
    EXPECT_TRUE(ins->IsConstantValue("[true,false,true]"));
    EXPECT_TRUE(ins->IsConstantValue("false"));                // from: = false
    EXPECT_TRUE(ins->IsConstantValue("true"));                 // from: != true
    EXPECT_TRUE(ins->IsConstantValue("10"));                   // from: >= 10
    EXPECT_TRUE(ins->IsConstantValue("10.123"));                   // from: >= 10
    EXPECT_TRUE(ins->IsConstantValue("\"PerceptionNode\""));   // from: = "PerceptionNode"

    EXPECT_FALSE(ins->IsConstantValue("LifecycleDiagReport.service_list[].service_state"));           // variable
    EXPECT_FALSE(ins->IsConstantValue("LifecycleDiagReport.initialized"));           // variable
    EXPECT_FALSE(ins->IsConstantValue("LifecycleDiagReport.services[0].name"));           // variable
    EXPECT_FALSE(ins->IsConstantValue("other_var"));           // variable
    EXPECT_FALSE(ins->IsConstantValue("\"Unclosed string"));    // malformed string
    EXPECT_FALSE(ins->IsConstantValue("[1, two]"));            // invalid list with non-integer
    EXPECT_FALSE(ins->IsConstantValue("GetStatus()"));         // function call
    EXPECT_FALSE(ins->IsConstantValue("{1: 2}"));              // not a supported constant format
}

TEST_F(ConditionEvaluatorTest, IsArrayValue_TC001) {
    EXPECT_TRUE(ins->IsArrayValue("[ 1,  2 ,232 ,23] "));
    EXPECT_TRUE(ins->IsArrayValue("[true, false, true]"));
    EXPECT_TRUE(ins->IsArrayValue("[\"hello\", \"good\", \"wodl\"]"));
    EXPECT_TRUE(ins->IsArrayValue("[1.23, 2.32, 29.32]"));
    EXPECT_FALSE(ins->IsArrayValue("false "));
    EXPECT_FALSE(ins->IsArrayValue("true "));
    EXPECT_FALSE(ins->IsArrayValue("10 "));
    EXPECT_FALSE(ins->IsArrayValue("10.123 "));
    EXPECT_FALSE(ins->IsArrayValue(" \"PerceptionNode\" "));
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandTypeFromConstantValue_TC001) {
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("42"), ConditionOperandType::INTEGER);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("-1"), ConditionOperandType::INTEGER);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("  100  "), ConditionOperandType::INTEGER);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("3.14"), ConditionOperandType::FLOATING);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("-0.001"), ConditionOperandType::FLOATING);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("  2.718 "), ConditionOperandType::FLOATING);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("true"), ConditionOperandType::BOOLEAN);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("FALSE"), ConditionOperandType::BOOLEAN); // case-insensitive
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("false"), ConditionOperandType::BOOLEAN); // case-insensitive

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("\"hello\""), ConditionOperandType::STRING);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("\"\""), ConditionOperandType::STRING);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("\"123\""), ConditionOperandType::STRING);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[1,2,3]"), ConditionOperandType::INTEGER);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[  -5 , 100 ]"), ConditionOperandType::INTEGER);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[1.0, 2.5, 3.14]"), ConditionOperandType::FLOATING);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[ 0.0 , -1.1 , 2.2 ]"), ConditionOperandType::FLOATING);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[true,false,true]"), ConditionOperandType::BOOLEAN);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[  true , FALSE ]"), ConditionOperandType::BOOLEAN);

    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("abc"), ConditionOperandType::UNKNOWN);
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("[1, \"two\"]"), ConditionOperandType::UNKNOWN); // mixed types
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("{1: 2}"), ConditionOperandType::UNKNOWN);        // object-like
    EXPECT_EQ(ins->EvaluateOperandTypeFromConstantValue("1.2.3"), ConditionOperandType::UNKNOWN);        // invalid float
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperator_NumericComparisons) {
    EXPECT_TRUE(ins->EvaluateOperandByOperator(10, 10, ConditionOperatorType::EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator(10, 10, ConditionOperatorType::NOT_EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator(10, -10, ConditionOperatorType::EQUAL));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(10, 9, ConditionOperatorType::NOT_EQUAL));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(10, 5, ConditionOperatorType::ABOVE));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(5.5, 5.0, ConditionOperatorType::EQUAL_ABOVE));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(5.5, 5.5, ConditionOperatorType::EQUAL_ABOVE));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(5.0, 5.5, ConditionOperatorType::LESS));
    EXPECT_FALSE(ins->EvaluateOperandByOperator(6.0, 5.5, ConditionOperatorType::LESS));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(5.0, 5.5, ConditionOperatorType::EQUAL_LESS));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(5.5, 5.5, ConditionOperatorType::EQUAL_LESS));
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperator_StringComparisons) {
    EXPECT_TRUE(ins->EvaluateOperandByOperator<std::string>("abc", "abc", ConditionOperatorType::EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator<std::string>("abc", "xyz", ConditionOperatorType::EQUAL));
    EXPECT_TRUE(ins->EvaluateOperandByOperator<std::string>("abc", "xyz", ConditionOperatorType::NOT_EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator<std::string>("xyz", "xyz", ConditionOperatorType::NOT_EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator<std::string>("abc", "xyz", ConditionOperatorType::LESS)); // unsupported
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperator_BooleanComparisons) {
    EXPECT_TRUE(ins->EvaluateOperandByOperator(true, true, ConditionOperatorType::EQUAL));
    EXPECT_TRUE(ins->EvaluateOperandByOperator(true, false, ConditionOperatorType::NOT_EQUAL));
    EXPECT_FALSE(ins->EvaluateOperandByOperator(false, true, ConditionOperatorType::EQUAL));
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperatorWithList_Int) {
    std::string values = "[1, 2 , 3 ]";
    EXPECT_TRUE(ins->EvaluateOperandByOperatorWithList(2, values, ConditionOperatorType::IN));
    EXPECT_FALSE(ins->EvaluateOperandByOperatorWithList(4, values, ConditionOperatorType::IN));
    EXPECT_TRUE(ins->EvaluateOperandByOperatorWithList(4, values, ConditionOperatorType::NOT_IN));
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperatorWithList_String) {
    std::string values = R"(["a", "b", "c"])";
    EXPECT_TRUE(ins->EvaluateOperandByOperatorWithList(std::string("b"), values, ConditionOperatorType::IN));
    EXPECT_FALSE(ins->EvaluateOperandByOperatorWithList(std::string("z"), values, ConditionOperatorType::IN));
}

TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperatorWithList_Bool) {
    std::string values = "[true]";
    EXPECT_TRUE(ins->EvaluateOperandByOperatorWithList(true, values, ConditionOperatorType::IN));
    EXPECT_FALSE(ins->EvaluateOperandByOperatorWithList(false, values, ConditionOperatorType::IN));
}

TEST_F(ConditionEvaluatorTest, ParseArrayValue_TC001) {
    auto result = ins->ParseArrayValue<int>("[1, 2, 3, 2 ]");
    std::set<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST_F(ConditionEvaluatorTest, ParseFloatArray) {
    auto result = ins->ParseArrayValue<double>(" [1.5 , 2.0 , 1.5]");
    std::set<double> expected = {1.5, 2.0};
    EXPECT_EQ(result, expected);
}

TEST_F(ConditionEvaluatorTest, ParseBooleanArray) {
    auto result = ins->ParseArrayValue<bool>("[true , false, true] ");
    std::set<bool> expected = {true, false};
    EXPECT_EQ(result, expected);
}

TEST_F(ConditionEvaluatorTest, ParseStringArray) {
    auto result = ins->ParseArrayValue<std::string>(R"(["apple", "banana", "apple"])");
    std::set<std::string> expected = {"apple", "banana"};
    EXPECT_EQ(result, expected);
}

TEST_F(ConditionEvaluatorTest, ParseEmptyArray) {
    auto result = ins->ParseArrayValue<int>("[]");
    EXPECT_TRUE(result.empty());
}

TEST_F(ConditionEvaluatorTest, ParseStringArray_ThrowsOnMissingQuotes) {
    EXPECT_EQ(0, ins->ParseArrayValue<std::string>(R"([apple, banana])").size());
}

TEST_F(ConditionEvaluatorTest, ParseBooleanArray_ThrowsOnInvalidKeyword) {
    EXPECT_EQ(0, ins->ParseArrayValue<bool>(R"([true, maybe])").size());
}

TEST_F(ConditionEvaluatorTest, FetchOrParseValue_Fetch) {
    AddTestValue("Sensor1", "temp", 25);
    AddTestValue("Sensor2", "angle", 123.25);
    AddTestValue("Sensor3", "enabled", true);
    AddTestValue("Sensor4", "name", "Actuator1");

    int64_t int_out;
    EXPECT_TRUE(ins->FetchOrParseValue("Sensor1.temp", int_out));
    EXPECT_EQ(25, int_out);
    double float_out;
    EXPECT_TRUE(ins->FetchOrParseValue("Sensor2.angle", float_out));
    EXPECT_EQ(123.25, float_out);
    bool bool_out;
    EXPECT_TRUE(ins->FetchOrParseValue("Sensor3.enabled", bool_out));
    EXPECT_EQ(true, bool_out);
    std::string str_out;
    EXPECT_TRUE(ins->FetchOrParseValue("Sensor4.name", str_out));
    EXPECT_EQ("Actuator1", str_out);
}

TEST_F(ConditionEvaluatorTest, FetchOrParseValue_Parse) {
    int64_t int_out;
    EXPECT_TRUE(ins->FetchOrParseValue("25", int_out));
    EXPECT_EQ(25, int_out);
    double float_out;
    EXPECT_TRUE(ins->FetchOrParseValue("123.25", float_out));
    EXPECT_EQ(123.25, float_out);
    bool bool_out;
    EXPECT_TRUE(ins->FetchOrParseValue("true", bool_out));
    EXPECT_EQ(true, bool_out);
    std::string str_out;
    EXPECT_TRUE(ins->FetchOrParseValue("\"Actuator1\"", str_out));
    EXPECT_EQ("Actuator1", str_out);
}

TEST_F(ConditionEvaluatorTest, ParseMalformedArray_Throws) {
    EXPECT_EQ(0, ins->ParseArrayValue<int>("1,2,3").size());  // no brackets
    EXPECT_EQ(0, ins->ParseArrayValue<int>("[1,2,3").size()); // missing ]
    EXPECT_EQ(0, ins->ParseArrayValue<int>("[1,,2]").size()); // double comma
}

// ==================== NEW TESTS FOR COVERAGE GAPS ====================

// Test for condition cache hit (lines 19-20)
TEST_F(ConditionEvaluatorTest, EvaluateCondition_CacheHit) {
    // First call parses and caches the condition
    EXPECT_TRUE(ins->EvaluateCondition("10 > 5"));
    // Second call should use the cached condition (covers line 19-20)
    EXPECT_TRUE(ins->EvaluateCondition("10 > 5"));
    // Verify consistent result
    EXPECT_TRUE(ins->EvaluateCondition("10 > 5"));
}

// Test for UNKNOWN operator type (lines 26-28) - invalid syntax
TEST_F(ConditionEvaluatorTest, EvaluateCondition_InvalidSyntax_ReturnsUnknownOperator) {
    // This malformed condition should fail regex match and return UNKNOWN operator type
    // Note: We need to test with something that parses but gives UNKNOWN operator
    // The regex might still match but operator won't be found
    EXPECT_FALSE(ins->EvaluateCondition("invalid_condition_without_operator"));
}

// Test for FLOATING case in IN/NOT_IN list evaluation (lines 49-54)
TEST_F(ConditionEvaluatorTest, EvaluateCondition_Floating_IN_List) {
    EXPECT_TRUE(ins->EvaluateCondition("3.14 in [1.0, 2.0, 3.14, 4.0]"));
    EXPECT_FALSE(ins->EvaluateCondition("5.5 in [1.0, 2.0, 3.0]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Floating_NOT_IN_List) {
    EXPECT_TRUE(ins->EvaluateCondition("9.99 not in [1.0, 2.0, 3.0]"));
    EXPECT_FALSE(ins->EvaluateCondition("2.0 not in [1.0, 2.0, 3.0]"));
}

TEST_F(ConditionEvaluatorTest, EvaluateCondition_Variable_Float_IN_List) {
    AddTestValue("Temperature", "value", 25.5);
    AddReferenceMessage("Temp", "Temperature", MessageProtocolType::DDS);
    EXPECT_TRUE(ins->EvaluateCondition("Temp.value in [20.0, 25.5, 30.0]"));
    EXPECT_FALSE(ins->EvaluateCondition("Temp.value in [10.0, 15.0, 20.0]"));
}

// Test for Tokenize function (lines 160-173)
TEST_F(ConditionEvaluatorTest, Tokenize_BasicFunctionality) {
    auto tokens = ins->Tokenize("a,b,c", ',');
    ASSERT_EQ(3u, tokens.size());
    EXPECT_EQ("a", tokens[0]);
    EXPECT_EQ("b", tokens[1]);
    EXPECT_EQ("c", tokens[2]);
}

TEST_F(ConditionEvaluatorTest, Tokenize_EmptyTokensSkipped) {
    auto tokens = ins->Tokenize("a,,b,", ',');
    ASSERT_EQ(2u, tokens.size());
    EXPECT_EQ("a", tokens[0]);
    EXPECT_EQ("b", tokens[1]);
}

TEST_F(ConditionEvaluatorTest, Tokenize_EmptyString) {
    auto tokens = ins->Tokenize("", ',');
    EXPECT_TRUE(tokens.empty());
}

TEST_F(ConditionEvaluatorTest, Tokenize_SingleElement) {
    auto tokens = ins->Tokenize("single", ',');
    ASSERT_EQ(1u, tokens.size());
    EXPECT_EQ("single", tokens[0]);
}

// Test for ConcatReleatedToken function (lines 175-202)
TEST_F(ConditionEvaluatorTest, ConcatReleatedToken_NotToken) {
    std::vector<std::string> tokens = {"not", "in", "abc"};
    ins->ConcatReleatedToken(tokens);
    ASSERT_EQ(2u, tokens.size());
    EXPECT_EQ("not in", tokens[0]);
    EXPECT_EQ("abc", tokens[1]);
}

TEST_F(ConditionEvaluatorTest, ConcatReleatedToken_NoRelatedTokens) {
    std::vector<std::string> tokens = {"a", "b", "c"};
    ins->ConcatReleatedToken(tokens);
    ASSERT_EQ(3u, tokens.size());
    EXPECT_EQ("a", tokens[0]);
    EXPECT_EQ("b", tokens[1]);
    EXPECT_EQ("c", tokens[2]);
}

TEST_F(ConditionEvaluatorTest, ConcatReleatedToken_EndsWithNot) {
    std::vector<std::string> tokens = {"a", "not"};
    ins->ConcatReleatedToken(tokens);
    // Should return early since "not" is at the end with no following token
    ASSERT_EQ(2u, tokens.size());
}

TEST_F(ConditionEvaluatorTest, ConcatReleatedToken_EmptyVector) {
    std::vector<std::string> tokens;
    ins->ConcatReleatedToken(tokens);
    EXPECT_TRUE(tokens.empty());
}

// Test for ConvertOperatorTypeToString function (lines 215-220)
TEST_F(ConditionEvaluatorTest, ConvertOperatorTypeToString_AllTypes) {
    EXPECT_EQ("=", ins->ConvertOperatorTypeToString(ConditionOperatorType::EQUAL));
    EXPECT_EQ("!=", ins->ConvertOperatorTypeToString(ConditionOperatorType::NOT_EQUAL));
    EXPECT_EQ(">=", ins->ConvertOperatorTypeToString(ConditionOperatorType::EQUAL_ABOVE));
    EXPECT_EQ("<=", ins->ConvertOperatorTypeToString(ConditionOperatorType::EQUAL_LESS));
    EXPECT_EQ(">", ins->ConvertOperatorTypeToString(ConditionOperatorType::ABOVE));
    EXPECT_EQ("<", ins->ConvertOperatorTypeToString(ConditionOperatorType::LESS));
    EXPECT_EQ("in", ins->ConvertOperatorTypeToString(ConditionOperatorType::IN));
    EXPECT_EQ("not in", ins->ConvertOperatorTypeToString(ConditionOperatorType::NOT_IN));
}

TEST_F(ConditionEvaluatorTest, ConvertOperatorTypeToString_Unknown) {
    EXPECT_EQ("", ins->ConvertOperatorTypeToString(ConditionOperatorType::UNKNOWN));
}

// Test for ConvertStringToOperatorType returning UNKNOWN (line 228)
TEST_F(ConditionEvaluatorTest, ConvertStringToOperatorType_Unknown) {
    EXPECT_EQ(ConditionOperatorType::UNKNOWN, ins->ConvertStringToOperatorType("invalid"));
    EXPECT_EQ(ConditionOperatorType::UNKNOWN, ins->ConvertStringToOperatorType("&&"));
    EXPECT_EQ(ConditionOperatorType::UNKNOWN, ins->ConvertStringToOperatorType("||"));
}

// Test for ConvertTypeIndexToOperandType with various types
TEST_F(ConditionEvaluatorTest, ConvertTypeIndexToOperandType_AllIntegerTypes) {
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(char)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(wchar_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(int8_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(int16_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(int32_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(int64_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(uint8_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(uint16_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(uint32_t)));
    EXPECT_EQ(ConditionOperandType::INTEGER, ins->ConvertTypeIndexToOperandType(typeid(uint64_t)));
}

TEST_F(ConditionEvaluatorTest, ConvertTypeIndexToOperandType_FloatingTypes) {
    EXPECT_EQ(ConditionOperandType::FLOATING, ins->ConvertTypeIndexToOperandType(typeid(float)));
    EXPECT_EQ(ConditionOperandType::FLOATING, ins->ConvertTypeIndexToOperandType(typeid(double)));
}

TEST_F(ConditionEvaluatorTest, ConvertTypeIndexToOperandType_BoolType) {
    EXPECT_EQ(ConditionOperandType::BOOLEAN, ins->ConvertTypeIndexToOperandType(typeid(bool)));
}

TEST_F(ConditionEvaluatorTest, ConvertTypeIndexToOperandType_StringTypes) {
    EXPECT_EQ(ConditionOperandType::STRING, ins->ConvertTypeIndexToOperandType(typeid(std::string)));
    EXPECT_EQ(ConditionOperandType::STRING, ins->ConvertTypeIndexToOperandType(typeid(std::wstring)));
}

TEST_F(ConditionEvaluatorTest, ConvertTypeIndexToOperandType_UnknownType) {
    // Test with a type that doesn't map to any operand type
    struct CustomType {};
    EXPECT_EQ(ConditionOperandType::UNKNOWN, ins->ConvertTypeIndexToOperandType(typeid(CustomType)));
}

// Test for EvaluateOperandByOperatorWithList with non-array value (lines 368-370)
TEST_F(ConditionEvaluatorTest, EvaluateOperandByOperatorWithList_NonArrayRH) {
    // When rh is not an array, should return false and log error
    EXPECT_FALSE(ins->EvaluateOperandByOperatorWithList(5, "not_an_array", ConditionOperatorType::IN));
    EXPECT_FALSE(ins->EvaluateOperandByOperatorWithList(5, "123", ConditionOperatorType::NOT_IN));
}

// Test for ResolveMessageName function
TEST_F(ConditionEvaluatorTest, ResolveMessageName_NoDotsInName) {
    // If name has no dot, should return as-is
    std::string result = ins->ResolveMessageName("SimpleName");
    EXPECT_EQ("SimpleName", result);
}

TEST_F(ConditionEvaluatorTest, ResolveMessageName_WithReferenceMessage) {
    AddReferenceMessage("RefMsg", "ActualMessage", MessageProtocolType::DDS);
    std::string result = ins->ResolveMessageName("RefMsg.field");
    EXPECT_EQ("ActualMessage.field", result);
}

TEST_F(ConditionEvaluatorTest, ResolveMessageName_NoMatchingReference) {
    // No reference message added, should return original name
    std::string result = ins->ResolveMessageName("Unknown.field");
    EXPECT_EQ("Unknown.field", result);
}

// Test for ParseArrayValue with invalid formats
TEST_F(ConditionEvaluatorTest, ParseArrayValue_MissingOpenBracket) {
    auto result = ins->ParseArrayValue<int>("1,2,3]");
    EXPECT_TRUE(result.empty());
}

TEST_F(ConditionEvaluatorTest, ParseArrayValue_InvalidIntElement) {
    auto result = ins->ParseArrayValue<int>("[1, abc, 3]");
    EXPECT_TRUE(result.empty());
}

TEST_F(ConditionEvaluatorTest, ParseArrayValue_DoubleArray) {
    auto result = ins->ParseArrayValue<double>("[1.5, 2.5, 3.5]");
    std::set<double> expected = {1.5, 2.5, 3.5};
    EXPECT_EQ(result, expected);
}

// ==================== END NEW TESTS ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
