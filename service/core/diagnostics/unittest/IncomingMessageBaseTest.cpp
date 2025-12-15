#include <gtest/gtest.h>

#include "executor/RequestBase.cpp"
#include "transmitter/request_msg/IncomingMessageBase.cpp"
#include "transmitter/Message.cpp"
#include "diagnostic/VariantData.cpp"

#include "mock/MockLogger.hpp"

using namespace diagnostic_manager;

class IncomingMessageBaseTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        ins = std::make_shared<IncomingMessageBase>();
        Mock_Logger = new MockLogger();
    }

    virtual void TearDown() override {
        ins.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    std::shared_ptr<IncomingMessageBase> ins;
};

// Test SetMessage and GetMessage
TEST_F(IncomingMessageBaseTest, SetMessage_TC001) {
    auto msg = std::make_shared<Message>();
    msg->SetName("test_message");
    
    ins->SetMessage(msg);
    
    auto retrieved = ins->GetMessage();
    ASSERT_NE(nullptr, retrieved);
    EXPECT_EQ("test_message", retrieved->GetName());
}

// Test GetMessage returns nullptr when no message is set
TEST_F(IncomingMessageBaseTest, GetMessage_TC001_NullByDefault) {
    auto retrieved = ins->GetMessage();
    EXPECT_EQ(nullptr, retrieved);
}

// Test SetMessage with nullptr
TEST_F(IncomingMessageBaseTest, SetMessage_TC002_Nullptr) {
    ins->SetMessage(nullptr);
    
    auto retrieved = ins->GetMessage();
    EXPECT_EQ(nullptr, retrieved);
}

// Test SetMessage overwrites previous message
TEST_F(IncomingMessageBaseTest, SetMessage_TC003_Overwrite) {
    auto msg1 = std::make_shared<Message>();
    msg1->SetName("first_message");
    
    auto msg2 = std::make_shared<Message>();
    msg2->SetName("second_message");
    
    ins->SetMessage(msg1);
    EXPECT_EQ("first_message", ins->GetMessage()->GetName());
    
    ins->SetMessage(msg2);
    EXPECT_EQ("second_message", ins->GetMessage()->GetName());
}

// Test multiple GetMessage calls return same message
TEST_F(IncomingMessageBaseTest, GetMessage_TC002_MultipleCallsReturnSame) {
    auto msg = std::make_shared<Message>();
    msg->SetName("persistent_message");
    
    ins->SetMessage(msg);
    
    auto first_get = ins->GetMessage();
    auto second_get = ins->GetMessage();
    
    EXPECT_EQ(first_get, second_get);
    EXPECT_EQ("persistent_message", first_get->GetName());
    EXPECT_EQ("persistent_message", second_get->GetName());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
