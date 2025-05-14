#include <gtest/gtest.h>

#include "executor/RequestBase.cpp"

using namespace diagnostic_manager;

class RequestBaseTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new RequestBase();
    }
    virtual void TearDown(){
        delete ins;
    }

    RequestBase* ins;
};

TEST_F(RequestBaseTest, GetRequestId_TC001) {
    EXPECT_EQ(0x01, ins->GetRequestId());
}

TEST_F(RequestBaseTest, GetRequestId_TC002_increasing_id) {
    EXPECT_EQ(0x02, ins->GetRequestId());

    RequestBase req3;
    EXPECT_EQ(0x03, req3.GetRequestId());
    RequestBase req4;
    EXPECT_EQ(0x04, req4.GetRequestId());
    RequestBase req5;
    EXPECT_EQ(0x05, req5.GetRequestId());
}

TEST_F(RequestBaseTest, GetRequestedTime_TC001) {
    auto current = std::chrono::steady_clock::now();
    auto elapsed = current - ins->GetRequestedTime();
    EXPECT_TRUE(std::chrono::milliseconds(-10) < elapsed && elapsed < std::chrono::milliseconds(10));
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
