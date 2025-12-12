// Unit tests for DDSMessageEndPoint class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the actual implementation - CMakeLists compiles it separately
#include "transmitter/DDSMessageEndPoint.hpp"
#include "unittest/mock/mock_message_endpoint.hpp"  // Provides MessageEndPoint implementation
#include "unittest/mock/mock_logger.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

using namespace lifecycle_manager;

// Concrete implementation for testing
class TestDDSMessageEndPoint : public DDSMessageEndPoint {
public:
    TestDDSMessageEndPoint(const std::string& topic) : DDSMessageEndPoint(topic) {}
    ~TestDDSMessageEndPoint() override = default;

    // Only override the pure virtual methods from MessageEndPoint
    void OnDataPublished(bool ret) override { data_published_called_ = true; data_published_result_ = ret; }
    void OnDataAvailable() override { data_available_called_ = true; }

    // Test helpers
    bool WasDataPublishedCalled() const { return data_published_called_; }
    bool GetDataPublishedResult() const { return data_published_result_; }
    bool WasDataAvailableCalled() const { return data_available_called_; }

private:
    bool data_published_called_ = false;
    bool data_published_result_ = false;
    bool data_available_called_ = false;
};

class DDSMessageEndPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        endpoint_ = std::make_unique<TestDDSMessageEndPoint>("test_topic");
    }

    void TearDown() override {
        endpoint_.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }

    std::unique_ptr<TestDDSMessageEndPoint> endpoint_;
};

TEST_F(DDSMessageEndPointTest, ConstructorDestructor) {
    TestDDSMessageEndPoint ep("another_topic");
    EXPECT_EQ(ep.GetTopic(), "another_topic");
}

TEST_F(DDSMessageEndPointTest, GetTopic) {
    EXPECT_EQ(endpoint_->GetTopic(), "test_topic");
}

TEST_F(DDSMessageEndPointTest, GetTypeName_WhenNoTypeSupport) {
    // When type_support_ is null, GetTypeName() should return empty string
    EXPECT_EQ(endpoint_->GetTypeName(), "");
}

TEST_F(DDSMessageEndPointTest, GetTypeSupport_Null) {
    EXPECT_EQ(endpoint_->GetTypeSupport(), nullptr);
}

TEST_F(DDSMessageEndPointTest, OnDataAvailable) {
    EXPECT_FALSE(endpoint_->WasDataAvailableCalled());
    endpoint_->OnDataAvailable();
    EXPECT_TRUE(endpoint_->WasDataAvailableCalled());
}

TEST_F(DDSMessageEndPointTest, OnDataPublished_Success) {
    EXPECT_FALSE(endpoint_->WasDataPublishedCalled());
    endpoint_->OnDataPublished(true);
    EXPECT_TRUE(endpoint_->WasDataPublishedCalled());
    EXPECT_TRUE(endpoint_->GetDataPublishedResult());
}

TEST_F(DDSMessageEndPointTest, OnDataPublished_Failure) {
    endpoint_->OnDataPublished(false);
    EXPECT_TRUE(endpoint_->WasDataPublishedCalled());
    EXPECT_FALSE(endpoint_->GetDataPublishedResult());
}

TEST_F(DDSMessageEndPointTest, OnPublicationMatched_Increment) {
    PublicationMatchedStatus status;
    status.current_count_change = 1;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnPublicationMatched_Decrement) {
    PublicationMatchedStatus status;
    status.current_count_change = -1;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnPublicationMatched_InvalidChange) {
    // Test the else branch with invalid value
    PublicationMatchedStatus status;
    status.current_count_change = 2;  // Not 1 or -1
    status.current_count = 2;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnPublicationMatched_ZeroChange) {
    PublicationMatchedStatus status;
    status.current_count_change = 0;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnSubscriptionMatched_Increment) {
    SubscriptionMatchedStatus status;
    status.current_count_change = 1;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnSubscriptionMatched_Decrement) {
    SubscriptionMatchedStatus status;
    status.current_count_change = -1;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnSubscriptionMatched_InvalidChange) {
    // Test the else branch with invalid value
    SubscriptionMatchedStatus status;
    status.current_count_change = 5;  // Not 1 or -1
    status.current_count = 5;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, OnSubscriptionMatched_ZeroChange) {
    SubscriptionMatchedStatus status;
    status.current_count_change = 0;
    status.current_count = 1;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndPointTest, on_data_available_Callback) {
    EXPECT_NO_THROW(endpoint_->on_data_available(nullptr));
    EXPECT_TRUE(endpoint_->WasDataAvailableCalled());
}

TEST_F(DDSMessageEndPointTest, Finalize_WithoutInitialize) {
    EXPECT_NO_THROW(endpoint_->Finalize());
}

TEST_F(DDSMessageEndPointTest, is_stopped_ReturnsFalse) {
    EXPECT_FALSE(endpoint_->is_stopped());
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
