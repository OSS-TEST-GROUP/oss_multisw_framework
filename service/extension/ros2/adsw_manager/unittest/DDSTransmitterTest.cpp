#include <gtest/gtest.h>
#include "transmitter/DDSTransmitter.cpp"  // Include implementation directl
#include "transmitter/DDSMessageEndpoint.cpp"
#include "logger/Logger.cpp"

#include "mock/MockDDSMessageEndpoint.hpp"

using namespace ros2::adsw_manager;

class DDSTransmitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ins = std::make_unique<DDSTransmitter>();
    }

    void TearDown() override {
        ins.reset();
    }

    std::unique_ptr<DDSTransmitter> ins;
};

TEST_F(DDSTransmitterTest, InitializeFinalize_TC001) {
    // Act
    bool result = ins->Initialize();

    // Assert
    ASSERT_TRUE(result);
    EXPECT_NE(ins->GetDomain(), nullptr);
    EXPECT_NE(ins->GetPublisher(), nullptr);
    EXPECT_NE(ins->GetSubscriber(), nullptr);

    ins->Finalize();
}

TEST_F(DDSTransmitterTest, AddMessageEndpoint_TC001) {
    std::shared_ptr<MockDDSMessageEndpoint> endpoint = std::make_shared<MockDDSMessageEndpoint>();
    
    EXPECT_TRUE(ins->AddMessageEndpoint(endpoint));
}

TEST_F(DDSTransmitterTest, GetMessageEndPoint_TC001) {
    std::shared_ptr<MockDDSMessageEndpoint> endpoint = std::make_shared<MockDDSMessageEndpoint>();
    
    EXPECT_TRUE(ins->AddMessageEndpoint(endpoint));
    EXPECT_TRUE(ins->Initialize());

    auto ret = ins->GetMessageEndpoint("TestTopic");
    ASSERT_EQ(ret, endpoint);
    EXPECT_TRUE(ret->participant_ != nullptr);
    EXPECT_TRUE(ret->publisher_ != nullptr);
    EXPECT_TRUE(ret->subscriber_ != nullptr);
    EXPECT_TRUE(ret->dds_topic_ != nullptr);
    EXPECT_TRUE(ret->data_reader_ != nullptr);
    EXPECT_TRUE(ret->data_writer_ != nullptr);
}


TEST_F(DDSTransmitterTest, GetMessageEndPoint_TC002_null) {
    auto ret = ins->GetMessageEndpoint("TestTopic");
    EXPECT_TRUE(ret == nullptr);
}

TEST_F(DDSTransmitterTest, GetDomain_TC001) {
    bool result = ins->Initialize();
    EXPECT_TRUE(ins->GetDomain() != nullptr);
}

TEST_F(DDSTransmitterTest, GetPublisher_TC001) {
    bool result = ins->Initialize();
    EXPECT_TRUE(ins->GetPublisher() != nullptr);
}

TEST_F(DDSTransmitterTest, GetSubscriber_TC001) {
    bool result = ins->Initialize();
    EXPECT_TRUE(ins->GetSubscriber() != nullptr);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
