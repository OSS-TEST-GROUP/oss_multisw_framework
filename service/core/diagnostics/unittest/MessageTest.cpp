#include <gtest/gtest.h>

#include "transmitter/Message.cpp"
#include "mock/VariantData_mock.hpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class MessageTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new Message();
    }
    virtual void TearDown(){
        delete ins;
    }

    Message* ins;
};

TEST_F(MessageTest, SetProtocolType_TC001) {
    ins->SetProtocolType(MessageProtocolType::DDS);
    EXPECT_EQ(MessageProtocolType::DDS, ins->protocol_);
}


TEST_F(MessageTest, GetProtocolType_TC001) {
    ins->protocol_ = MessageProtocolType::DDS;
    EXPECT_EQ(MessageProtocolType::DDS, ins->GetProtocolType());
}

TEST_F(MessageTest, SetTypeName_TC001) {
    ins->SetTypeName("msg_type");
    EXPECT_EQ("msg_type", ins->type_name_);
}

TEST_F(MessageTest, GetTypeName_TC001) {
    ins->type_name_ = "msg_type";
    EXPECT_EQ("msg_type", ins->GetTypeName());
}

TEST_F(MessageTest, SetData_TC001) {
    auto data = std::make_shared<VariantData>();
    ins->SetData(data);
    EXPECT_EQ(data, ins->data_);
}

TEST_F(MessageTest, GetData_TC001) {
    auto data = std::make_shared<VariantData>();
    ins->data_ = data;
    EXPECT_EQ(data, ins->GetData());
}

TEST_F(MessageTest, SetIdentifier_TC001) {
    ins->SetIdentifier("msg_id");
    EXPECT_EQ("msg_id", ins->identifier_);
}

TEST_F(MessageTest, GetIdentifier_TC001) {
    ins->identifier_ = "msg_id";
    EXPECT_EQ("msg_id", ins->GetIdentifier());
}

TEST_F(MessageTest, GetUpdateTime_TC001) {
    MessageTime msg_time = ins->GetUpdateTime();
    MessageTime current = std::chrono::steady_clock::now();
    auto diff = msg_time - current;
    EXPECT_TRUE(diff < 1ms && diff > -1ms);
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
