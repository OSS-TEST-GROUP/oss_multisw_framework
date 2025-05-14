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

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
