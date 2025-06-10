#include <gtest/gtest.h>

#include "diagnostic_analyzer/ReferenceMessage.cpp"
#include "transmitter/MessageProtocolType.hpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class ReferenceMessageTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new ReferenceMessage("TestMessage", "This is a test message", MessageProtocolType::DDS);
    }
    virtual void TearDown(){
        delete ins;
    }

    ReferenceMessage* ins;
};

TEST_F(ReferenceMessageTest, GetName_TC001) {
    EXPECT_EQ(ins->GetName(), "TestMessage");
}

TEST_F(ReferenceMessageTest, GetMessage_TC002) {
    EXPECT_EQ(ins->GetMessage(), "This is a test message");
}

TEST_F(ReferenceMessageTest, GetProtocol_TC003) {
    EXPECT_EQ(ins->GetProtocol(), MessageProtocolType::DDS);
}

TEST_F(ReferenceMessageTest, SetAndCheckDiagCode_TC004) {
    ins->SetDiagCode(101);
    EXPECT_TRUE(ins->CheckDiagCode(101));
    EXPECT_FALSE(ins->CheckDiagCode(202));
}

TEST_F(ReferenceMessageTest, RemoveDiagCode_TC005) {
    ins->SetDiagCode(303);
    EXPECT_TRUE(ins->CheckDiagCode(303));
    ins->RemoveDiagCode(303);
    EXPECT_FALSE(ins->CheckDiagCode(303));
}

TEST_F(ReferenceMessageTest, GetAllDiagCode_TC006) {
    ins->SetDiagCode(1);
    ins->SetDiagCode(2);
    ins->SetDiagCode(3);
    auto codes = ins->GetAllDiagCode();
    EXPECT_EQ(codes.size(), 3);
    EXPECT_TRUE(codes.count(1));
    EXPECT_TRUE(codes.count(2));
    EXPECT_TRUE(codes.count(3));
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
