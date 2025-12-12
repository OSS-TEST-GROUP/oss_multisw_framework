// Unit tests for DDSMessageEndpoint class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include header only - implementation is compiled separately via CMakeLists.txt
#include "transmitter/DDSMessageEndpoint.hpp"

#include "mock/MockLogger.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

using namespace ros2::adsw_manager;

// Concrete implementation for testing
class TestDDSMessageEndpoint : public DDSMessageEndpoint {
public:
    TestDDSMessageEndpoint() = default;
    ~TestDDSMessageEndpoint() override = default;

    std::string GetTopic() override { return topic_; }
    std::string GetTypeName() override { return type_name_; }
    std::shared_ptr<TypeSupport> GetTypeSupport() override { return type_support_; }
    void OnDataAvailable() override { on_data_available_called_ = true; }

    // Test helpers
    void SetTopic(const std::string& topic) { topic_ = topic; }
    void SetTypeName(const std::string& type_name) { type_name_ = type_name; }
    void SetTypeSupport(std::shared_ptr<TypeSupport> ts) { type_support_ = ts; }
    bool WasOnDataAvailableCalled() const { return on_data_available_called_; }

private:
    std::string topic_ = "test_topic";
    std::string type_name_ = "test_type";
    std::shared_ptr<TypeSupport> type_support_;
    bool on_data_available_called_ = false;
};

class DDSMessageEndpointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        endpoint_ = std::make_unique<TestDDSMessageEndpoint>();
    }

    void TearDown() override {
        endpoint_.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    std::unique_ptr<TestDDSMessageEndpoint> endpoint_;
};

TEST_F(DDSMessageEndpointTest, ConstructorDestructor) {
    TestDDSMessageEndpoint ep;
    // Just verify no crash
}

TEST_F(DDSMessageEndpointTest, GetTopic) {
    endpoint_->SetTopic("my_custom_topic");
    EXPECT_EQ(endpoint_->GetTopic(), "my_custom_topic");
}

TEST_F(DDSMessageEndpointTest, GetTypeName) {
    endpoint_->SetTypeName("my_custom_type");
    EXPECT_EQ(endpoint_->GetTypeName(), "my_custom_type");
}

TEST_F(DDSMessageEndpointTest, GetTypeSupport_Null) {
    EXPECT_EQ(endpoint_->GetTypeSupport(), nullptr);
}

TEST_F(DDSMessageEndpointTest, OnDataAvailable) {
    EXPECT_FALSE(endpoint_->WasOnDataAvailableCalled());
    endpoint_->OnDataAvailable();
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
}

TEST_F(DDSMessageEndpointTest, Initialize_NullParticipant) {
    // Initialize with null participant should fail
    EXPECT_FALSE(endpoint_->Initialize(nullptr, nullptr, nullptr));
}

TEST_F(DDSMessageEndpointTest, Write_NullDataWriter) {
    // Write without initialization should fail
    int dummy_data = 42;
    EXPECT_FALSE(endpoint_->Write(&dummy_data));
}

TEST_F(DDSMessageEndpointTest, GetPendingReadData_NullReader) {
    // Without initialization, should return 0
    EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
}

TEST_F(DDSMessageEndpointTest, GetPendingWriteData_NullWriter) {
    // Without initialization, should return 0
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
}

TEST_F(DDSMessageEndpointTest, Finalize_WithoutInitialize) {
    // Should not crash
    EXPECT_NO_THROW(endpoint_->Finalize());
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_Increment) {
    PublicationMatchedStatus status;
    status.current_count_change = 1;
    status.current_count = 1;
    
    // Should not crash even with null writer
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_Decrement) {
    PublicationMatchedStatus status;
    status.current_count_change = -1;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_InvalidChange) {
    // Test for the else branch (invalid current_count_change value)
    PublicationMatchedStatus status;
    status.current_count_change = 2;  // Not 1 or -1
    status.current_count = 2;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_ZeroChange) {
    // Test for the else branch with zero change
    PublicationMatchedStatus status;
    status.current_count_change = 0;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_Increment) {
    SubscriptionMatchedStatus status;
    status.current_count_change = 1;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_Decrement) {
    SubscriptionMatchedStatus status;
    status.current_count_change = -1;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_InvalidChange) {
    // Test for the else branch (invalid current_count_change value)
    SubscriptionMatchedStatus status;
    status.current_count_change = 5;  // Not 1 or -1
    status.current_count = 5;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_ZeroChange) {
    // Test for the else branch with zero change
    SubscriptionMatchedStatus status;
    status.current_count_change = 0;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, on_data_available_Callback) {
    // This tests the DataReaderListener callback
    EXPECT_NO_THROW(endpoint_->on_data_available(nullptr));
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
