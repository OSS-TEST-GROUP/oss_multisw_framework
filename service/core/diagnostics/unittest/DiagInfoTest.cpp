#include <gtest/gtest.h>

#include "diagnostic/DiagInfo.cpp"

using namespace diagnostic_manager;

class DiagInfoTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DiagInfo(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    }
    virtual void TearDown(){
        delete ins;
    }

    DiagInfo* ins;
};

TEST_F(DiagInfoTest, GetDiagCode_TC001) {
    EXPECT_EQ(0x123, ins->GetDiagCode());
}

TEST_F(DiagInfoTest, GetDiagName_TC001) {
    EXPECT_EQ("TestDiag1", ins->GetDiagName());
}

TEST_F(DiagInfoTest, GetStatus_TC001) {
    EXPECT_EQ(DiagInfoStatus::PASS, ins->GetStatus());
}

TEST_F(DiagInfoTest, GetMessage_TC001) {
    EXPECT_EQ("TestMessage1", ins->GetMessage());
}

TEST_F(DiagInfoTest, GetUpdateTime_TC001) {
    auto current = std::chrono::steady_clock::now();
    auto timepoint = ins->GetUpdateTime();

    auto elapsed = current - timepoint;
    
    EXPECT_TRUE(std::chrono::milliseconds(-10) < elapsed && elapsed < std::chrono::milliseconds(10));
}

TEST_F(DiagInfoTest, DefaultConstructor_TC002) {
    DiagInfo default_diag;
    EXPECT_EQ(0, default_diag.GetDiagCode());
    EXPECT_EQ("", default_diag.GetDiagName());
    EXPECT_EQ("", default_diag.GetMessage());
}

TEST_F(DiagInfoTest, OperatorEquals_SameValues_TC003) {
    DiagInfo diag1(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    DiagInfo diag2(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    EXPECT_TRUE(diag1 == diag2);
}

TEST_F(DiagInfoTest, OperatorEquals_DifferentCode_TC004) {
    DiagInfo diag1(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    DiagInfo diag2(0x124, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    EXPECT_FALSE(diag1 == diag2);
}

TEST_F(DiagInfoTest, OperatorEquals_DifferentName_TC005) {
    DiagInfo diag1(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    DiagInfo diag2(0x123, "TestDiag2", DiagInfoStatus::PASS, "TestMessage1");
    EXPECT_FALSE(diag1 == diag2);
}

TEST_F(DiagInfoTest, OperatorEquals_DifferentStatus_TC006) {
    DiagInfo diag1(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    DiagInfo diag2(0x123, "TestDiag1", DiagInfoStatus::FATAL, "TestMessage1");
    EXPECT_FALSE(diag1 == diag2);
}

TEST_F(DiagInfoTest, OperatorEquals_DifferentMessage_TC007) {
    DiagInfo diag1(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage1");
    DiagInfo diag2(0x123, "TestDiag1", DiagInfoStatus::PASS, "TestMessage2");
    EXPECT_FALSE(diag1 == diag2);
}

TEST_F(DiagInfoTest, AllDiagInfoStatuses_TC008) {
    DiagInfo diag_pass(0x1, "test", DiagInfoStatus::PASS, "msg");
    DiagInfo diag_fatal(0x2, "test", DiagInfoStatus::FATAL, "msg");
    DiagInfo diag_warning(0x3, "test", DiagInfoStatus::WARNING, "msg");
    DiagInfo diag_unknown(0x4, "test", DiagInfoStatus::UNKNOWN, "msg");
    
    EXPECT_EQ(DiagInfoStatus::PASS, diag_pass.GetStatus());
    EXPECT_EQ(DiagInfoStatus::FATAL, diag_fatal.GetStatus());
    EXPECT_EQ(DiagInfoStatus::WARNING, diag_warning.GetStatus());
    EXPECT_EQ(DiagInfoStatus::UNKNOWN, diag_unknown.GetStatus());
}

TEST_F(DiagInfoTest, EmptyStrings_TC009) {
    DiagInfo diag(0, "", DiagInfoStatus::PASS, "");
    EXPECT_EQ("", diag.GetDiagName());
    EXPECT_EQ("", diag.GetMessage());
}

TEST_F(DiagInfoTest, LongStrings_TC010) {
    std::string long_name(1000, 'a');
    std::string long_message(5000, 'b');
    DiagInfo diag(0x999, long_name, DiagInfoStatus::PASS, long_message);
    EXPECT_EQ(long_name, diag.GetDiagName());
    EXPECT_EQ(long_message, diag.GetMessage());
}

TEST_F(DiagInfoTest, SpecialCharacters_TC011) {
    DiagInfo diag(0xABCD, "Diag-Name_123.test", DiagInfoStatus::WARNING, "Message with special chars: !@#$%^&*()");
    EXPECT_EQ("Diag-Name_123.test", diag.GetDiagName());
    EXPECT_EQ("Message with special chars: !@#$%^&*()", diag.GetMessage());
}

TEST_F(DiagInfoTest, NegativeCode_TC012) {
    DiagInfo diag(-123, "test", DiagInfoStatus::FATAL, "msg");
    EXPECT_EQ(-123, diag.GetDiagCode());
}

TEST_F(DiagInfoTest, ZeroCode_TC013) {
    DiagInfo diag(0, "test", DiagInfoStatus::PASS, "msg");
    EXPECT_EQ(0, diag.GetDiagCode());
}

TEST_F(DiagInfoTest, LargeCode_TC014) {
    DiagInfo diag(0x7FFFFFFF, "test", DiagInfoStatus::UNKNOWN, "msg");
    EXPECT_EQ(0x7FFFFFFF, diag.GetDiagCode());
}

TEST_F(DiagInfoTest, SourceMessageName_TC015) {
    EXPECT_EQ("", ins->GetSourceMessageName());
    ins->SetSourceMessageName("sensor_msg_1");
    EXPECT_EQ("sensor_msg_1", ins->GetSourceMessageName());
}

TEST_F(DiagInfoTest, RawData_TC016) {
    EXPECT_EQ("", ins->GetRawData());
    ins->SetRawData("raw_hex_data_123");
    EXPECT_EQ("raw_hex_data_123", ins->GetRawData());
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

