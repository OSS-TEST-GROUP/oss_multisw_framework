#include <gtest/gtest.h>

#include "transmitter/Message.cpp"
#include "diagnostic/VariantData.cpp"
#include "mock/MockLogger.hpp"

using namespace diagnostic_manager;
using namespace std::chrono_literals;

class MessageTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new Message();
        Mock_Logger = new MockLogger();
    }
    virtual void TearDown(){
        delete ins;
        delete Mock_Logger;
        Mock_Logger = nullptr;
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

TEST_F(MessageTest, ParameterizedConstructor_TC002) {
    auto data = std::make_shared<VariantData>();
    Message msg("test_msg", MessageProtocolType::DDS, "test_type", data, "test_id");
    
    EXPECT_EQ("test_msg", msg.GetName());
    EXPECT_EQ(MessageProtocolType::DDS, msg.GetProtocolType());
    EXPECT_EQ("test_type", msg.GetTypeName());
    EXPECT_EQ(data, msg.GetData());
    EXPECT_EQ("test_id", msg.GetIdentifier());
}

TEST_F(MessageTest, SetName_TC003) {
    ins->SetName("new_name");
    EXPECT_EQ("new_name", ins->GetName());
}

TEST_F(MessageTest, GetName_TC004) {
    ins->name_ = "existing_name";
    EXPECT_EQ("existing_name", ins->GetName());
}

TEST_F(MessageTest, EmptyStrings_TC005) {
    ins->SetName("");
    ins->SetTypeName("");
    ins->SetIdentifier("");
    
    EXPECT_EQ("", ins->GetName());
    EXPECT_EQ("", ins->GetTypeName());
    EXPECT_EQ("", ins->GetIdentifier());
}

TEST_F(MessageTest, LongStrings_TC006) {
    std::string long_name(1000, 'a');
    std::string long_type(500, 'b');
    std::string long_id(200, 'c');
    
    ins->SetName(long_name);
    ins->SetTypeName(long_type);
    ins->SetIdentifier(long_id);
    
    EXPECT_EQ(long_name, ins->GetName());
    EXPECT_EQ(long_type, ins->GetTypeName());
    EXPECT_EQ(long_id, ins->GetIdentifier());
}

TEST_F(MessageTest, SpecialCharacters_TC007) {
    ins->SetName("msg-name_123.test");
    ins->SetTypeName("type!@#$%");
    ins->SetIdentifier("id/path/to/resource");
    
    EXPECT_EQ("msg-name_123.test", ins->GetName());
    EXPECT_EQ("type!@#$%", ins->GetTypeName());
    EXPECT_EQ("id/path/to/resource", ins->GetIdentifier());
}

TEST_F(MessageTest, NullDataPointer_TC008) {
    ins->SetData(nullptr);
    EXPECT_EQ(nullptr, ins->GetData());
}

TEST_F(MessageTest, ReplaceData_TC009) {
    auto data1 = std::make_shared<VariantData>();
    auto data2 = std::make_shared<VariantData>();
    
    ins->SetData(data1);
    EXPECT_EQ(data1, ins->GetData());
    
    ins->SetData(data2);
    EXPECT_EQ(data2, ins->GetData());
}

TEST_F(MessageTest, AllProtocolTypes_TC010) {
    ins->SetProtocolType(MessageProtocolType::DDS);
    EXPECT_EQ(MessageProtocolType::DDS, ins->GetProtocolType());
    
    // SOMEIP not defined in production enum; use DDS as representative
    ins->SetProtocolType(MessageProtocolType::DDS);
    EXPECT_EQ(MessageProtocolType::DDS, ins->GetProtocolType());
    
    // ROS2 support in production is represented by DDS_ROS
    ins->SetProtocolType(MessageProtocolType::DDS_ROS);
    EXPECT_EQ(MessageProtocolType::DDS_ROS, ins->GetProtocolType());
}

TEST_F(MessageTest, MultipleUpdates_TC011) {
    ins->SetName("name1");
    ins->SetName("name2");
    ins->SetName("name3");
    EXPECT_EQ("name3", ins->GetName());
    
    ins->SetTypeName("type1");
    ins->SetTypeName("type2");
    EXPECT_EQ("type2", ins->GetTypeName());
}

TEST_F(MessageTest, FullMessageSetup_TC012) {
    auto data = std::make_shared<VariantData>();
    data->SetName("data_name");
    
    ins->SetName("full_message");
    ins->SetProtocolType(MessageProtocolType::DDS_ROS);
    ins->SetTypeName("full_type");
    ins->SetData(data);
    ins->SetIdentifier("full_id");
    
    EXPECT_EQ("full_message", ins->GetName());
    EXPECT_EQ(MessageProtocolType::DDS_ROS, ins->GetProtocolType());
    EXPECT_EQ("full_type", ins->GetTypeName());
    EXPECT_EQ(data, ins->GetData());
    EXPECT_EQ("full_id", ins->GetIdentifier());
    EXPECT_EQ("data_name", ins->GetData()->GetName());
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
