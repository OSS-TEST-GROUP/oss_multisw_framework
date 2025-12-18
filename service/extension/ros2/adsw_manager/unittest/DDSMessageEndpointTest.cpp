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
        ON_CALL(*Mock_Logger, Get()).WillByDefault(Return(Mock_Logger));
           
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

// ============================================================================
// Additional Coverage Tests for Uncovered Code Paths
// ============================================================================

TEST_F(DDSMessageEndpointTest, Initialize_NullTypeSupport_ReturnsFalse) {
    // Mock DDS objects
    DomainParticipant* mock_participant = reinterpret_cast<DomainParticipant*>(0x1000);
    Publisher* mock_publisher = reinterpret_cast<Publisher*>(0x2000);
    Subscriber* mock_subscriber = reinterpret_cast<Subscriber*>(0x3000);
    
    // Set type support to nullptr
    endpoint_->SetTypeSupport(nullptr);
    
    // Initialize should fail and return false when type_support is null
    bool result = endpoint_->Initialize(mock_participant, mock_publisher, mock_subscriber);
    
    EXPECT_FALSE(result);
}

TEST_F(DDSMessageEndpointTest, Read_NullDataReader_ReturnsFalse) {
    // Test the Read template function with null data_reader_
    struct DummyMessage {
        int value;
    };
    
    DummyMessage msg;
    
    // Read should return false when data_reader_ is null
    bool result = endpoint_->Read(msg);
    
    EXPECT_FALSE(result);
}

TEST_F(DDSMessageEndpointTest, Read_MultipleCalls_AllReturnFalse) {
    // Test multiple calls to Read with null reader
    struct TestData {
        int id;
        std::string name;
    };
    
    TestData data1, data2, data3;
    
    EXPECT_FALSE(endpoint_->Read(data1));
    EXPECT_FALSE(endpoint_->Read(data2));
    EXPECT_FALSE(endpoint_->Read(data3));
}

TEST_F(DDSMessageEndpointTest, Write_BeforeInitialization_ReturnsFalse) {
    // Write should fail before initialization
    int test_value = 123;
    
    EXPECT_FALSE(endpoint_->Write(&test_value));
}

TEST_F(DDSMessageEndpointTest, Write_NullValue_ReturnsFalse) {
    // Write with null value should return false
    EXPECT_FALSE(endpoint_->Write(nullptr));
}

TEST_F(DDSMessageEndpointTest, GetPendingReadData_MultipleCallsBeforeInit) {
    // Multiple calls should consistently return 0
    EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
    EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
    EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
}

TEST_F(DDSMessageEndpointTest, GetPendingWriteData_MultipleCallsBeforeInit) {
    // Multiple calls should consistently return 0
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_MultipleIncrements) {
    // Test multiple increments
    PublicationMatchedStatus status;
    
    status.current_count_change = 1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    status.current_count_change = 1;
    status.current_count = 2;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    status.current_count_change = 1;
    status.current_count = 3;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_MultipleDecrements) {
    // Test multiple decrements
    SubscriptionMatchedStatus status;
    
    status.current_count_change = -1;
    status.current_count = 2;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
    
    status.current_count_change = -1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
    
    status.current_count_change = -1;
    status.current_count = 0;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_LargeCountChange) {
    // Test with large count change values
    PublicationMatchedStatus status;
    status.current_count_change = 100;
    status.current_count = 100;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_LargeNegativeCountChange) {
    // Test with large negative count change
    SubscriptionMatchedStatus status;
    status.current_count_change = -100;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, Finalize_MultipleCalls) {
    // Multiple Finalize calls should be safe
    EXPECT_NO_THROW(endpoint_->Finalize());
    EXPECT_NO_THROW(endpoint_->Finalize());
    EXPECT_NO_THROW(endpoint_->Finalize());
}

TEST_F(DDSMessageEndpointTest, Initialize_ThenFinalize_ThenUse) {
    // Attempt to use endpoint after finalization
    DomainParticipant* mock_participant = reinterpret_cast<DomainParticipant*>(0x1000);
    
    // Initialize with null type support (will fail)
    endpoint_->SetTypeSupport(nullptr);
    endpoint_->Initialize(mock_participant, nullptr, nullptr);
    
    // Finalize
    endpoint_->Finalize();
    
    // Try to use after finalize
    int data = 42;
    EXPECT_FALSE(endpoint_->Write(&data));
    EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
}

TEST_F(DDSMessageEndpointTest, SetAndGetTopic_EmptyString) {
    endpoint_->SetTopic("");
    EXPECT_EQ(endpoint_->GetTopic(), "");
}

TEST_F(DDSMessageEndpointTest, SetAndGetTypeName_EmptyString) {
    endpoint_->SetTypeName("");
    EXPECT_EQ(endpoint_->GetTypeName(), "");
}

TEST_F(DDSMessageEndpointTest, SetAndGetTopic_LongString) {
    std::string long_topic(1000, 'a');
    endpoint_->SetTopic(long_topic);
    EXPECT_EQ(endpoint_->GetTopic(), long_topic);
}

TEST_F(DDSMessageEndpointTest, OnDataAvailable_MultipleCalls) {
    // Test that OnDataAvailable can be called multiple times
    EXPECT_FALSE(endpoint_->WasOnDataAvailableCalled());
    
    endpoint_->OnDataAvailable();
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
    
    endpoint_->OnDataAvailable();
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
}

TEST_F(DDSMessageEndpointTest, on_data_available_MultipleCallbacks) {
    // Test multiple callbacks
    endpoint_->on_data_available(nullptr);
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
    
    // Reset for next test (would need to modify test class)
    // For now, just verify it doesn't crash
    endpoint_->on_data_available(nullptr);
    endpoint_->on_data_available(nullptr);
}

TEST_F(DDSMessageEndpointTest, Initialize_OnlyPublisher) {
    // Initialize with only publisher (no subscriber)
    DomainParticipant* mock_participant = reinterpret_cast<DomainParticipant*>(0x1000);
    Publisher* mock_publisher = reinterpret_cast<Publisher*>(0x2000);
    
    endpoint_->SetTypeSupport(nullptr);
    
    bool result = endpoint_->Initialize(mock_participant, mock_publisher, nullptr);
    
    EXPECT_FALSE(result);  // Should fail due to null type support
}

TEST_F(DDSMessageEndpointTest, Initialize_OnlySubscriber) {
    // Initialize with only subscriber (no publisher)
    DomainParticipant* mock_participant = reinterpret_cast<DomainParticipant*>(0x1000);
    Subscriber* mock_subscriber = reinterpret_cast<Subscriber*>(0x3000);
    
    endpoint_->SetTypeSupport(nullptr);
    
    bool result = endpoint_->Initialize(mock_participant, nullptr, mock_subscriber);
    
    EXPECT_FALSE(result);  // Should fail due to null type support
}

TEST_F(DDSMessageEndpointTest, Initialize_BothPublisherAndSubscriber) {
    // Initialize with both publisher and subscriber
    DomainParticipant* mock_participant = reinterpret_cast<DomainParticipant*>(0x1000);
    Publisher* mock_publisher = reinterpret_cast<Publisher*>(0x2000);
    Subscriber* mock_subscriber = reinterpret_cast<Subscriber*>(0x3000);
    
    endpoint_->SetTypeSupport(nullptr);
    
    bool result = endpoint_->Initialize(mock_participant, mock_publisher, mock_subscriber);
    
    EXPECT_FALSE(result);  // Should fail due to null type support
}

TEST_F(DDSMessageEndpointTest, GetTypeSupport_AfterSet) {
    // Create a mock type support (just a pointer for testing)
    auto mock_ts = std::make_shared<TypeSupport>();
    endpoint_->SetTypeSupport(mock_ts);
    
    EXPECT_EQ(endpoint_->GetTypeSupport(), mock_ts);
}

TEST_F(DDSMessageEndpointTest, Read_DifferentMessageTypes) {
    // Test Read with different message types
    
    struct MessageType1 {
        int field1;
        double field2;
    };
    
    struct MessageType2 {
        std::string text;
        bool flag;
    };
    
    MessageType1 msg1;
    MessageType2 msg2;
    
    // Both should return false with null reader
    EXPECT_FALSE(endpoint_->Read(msg1));
    EXPECT_FALSE(endpoint_->Read(msg2));
}

TEST_F(DDSMessageEndpointTest, ConstructorSetsDefaultValues) {
    // Verify constructor sets things up correctly
    TestDDSMessageEndpoint ep;
    
    EXPECT_EQ(ep.GetTopic(), "test_topic");
    EXPECT_EQ(ep.GetTypeName(), "test_type");
    EXPECT_EQ(ep.GetTypeSupport(), nullptr);
    EXPECT_FALSE(ep.WasOnDataAvailableCalled());
}

// ============================================================================
// Enhanced Coverage Tests for Error Branches and Edge Cases
// ============================================================================
// Note: Due to the tight coupling with Fast DDS, we cannot easily mock the
// DDS infrastructure to test error branches in Initialize(). However, we can
// still improve coverage by testing edge cases in the methods we can control.
// ============================================================================

TEST_F(DDSMessageEndpointTest, GetPendingWriteData_AlwaysReturnsZero) {
    // Test that GetPendingWriteData always returns 0 (as per implementation)
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
    
    // Even after attempts to initialize (which will fail without proper DDS setup)
    endpoint_->Initialize(nullptr, nullptr, nullptr);
    EXPECT_EQ(endpoint_->GetPendingWriteData(), 0);
}

TEST_F(DDSMessageEndpointTest, Read_Template_WithDifferentTypes) {
    // Test the Read template function with various types to ensure template
    // instantiation works correctly
    
    struct SimpleStruct {
        int id;
    };
    
    struct ComplexStruct {
        int id;
        double value;
        std::string name;
    };
    
    SimpleStruct simple;
    ComplexStruct complex;
    int primitive;
    
    // All should return false without a valid data_reader_
    EXPECT_FALSE(endpoint_->Read(simple));
    EXPECT_FALSE(endpoint_->Read(complex));
    EXPECT_FALSE(endpoint_->Read(primitive));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_BoundaryValues) {
    // Test with boundary values for current_count
    PublicationMatchedStatus status;
    
    // Max value
    status.current_count_change = 1;
    status.current_count = INT32_MAX;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    // Min value after decrement
    status.current_count_change = -1;
    status.current_count = 0;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_BoundaryValues) {
    // Test with boundary values for current_count
    SubscriptionMatchedStatus status;
    
    // Max value
    status.current_count_change = 1;
    status.current_count = INT32_MAX;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
    
    // Min value
    status.current_count_change = -1;
    status.current_count = 0;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_NegativeValues) {
    // Test with large negative count change values
    PublicationMatchedStatus status;
    status.current_count_change = -1000;
    status.current_count = 0;
    
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_LargePositiveValues) {
    // Test with large positive count change values
    SubscriptionMatchedStatus status;
    status.current_count_change = 1000;
    status.current_count = 1000;
    
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, GetPendingReadData_Consistency) {
    // Verify that GetPendingReadData returns 0 consistently without initialization
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(endpoint_->GetPendingReadData(), 0);
    }
}

TEST_F(DDSMessageEndpointTest, Write_RepeatedCalls) {
    // Test Write with repeated calls (all should return false without initialization)
    int data1 = 1, data2 = 2, data3 = 3;
    
    EXPECT_FALSE(endpoint_->Write(&data1));
    EXPECT_FALSE(endpoint_->Write(&data2));
    EXPECT_FALSE(endpoint_->Write(&data3));
}

TEST_F(DDSMessageEndpointTest, Finalize_IdempotencyTest) {
    // Test that Finalize can be called multiple times safely (idempotency)
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(endpoint_->Finalize());
    }
}

TEST_F(DDSMessageEndpointTest, OnDataAvailable_WithNonNullReader) {
    // Test on_data_available with non-null reader (though we can't use it)
    // This exercises the code path that leads to OnDataAvailable()
    DomainParticipant* fake_participant = reinterpret_cast<DomainParticipant*>(0x1234);
    DataReader* fake_reader = reinterpret_cast<DataReader*>(0x5678);
    
    EXPECT_FALSE(endpoint_->WasOnDataAvailableCalled());
    endpoint_->on_data_available(fake_reader);
    EXPECT_TRUE(endpoint_->WasOnDataAvailableCalled());
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched_SequenceOfEvents) {
    // Test a realistic sequence of publication matched events
    PublicationMatchedStatus status;
    
    // First subscriber connects
    status.current_count_change = 1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    // Second subscriber connects
    status.current_count_change = 1;
    status.current_count = 2;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    // First subscriber disconnects
    status.current_count_change = -1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
    
    // Second subscriber disconnects
    status.current_count_change = -1;
    status.current_count = 0;
    EXPECT_NO_THROW(endpoint_->on_publication_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched_SequenceOfEvents) {
    // Test a realistic sequence of subscription matched events
    SubscriptionMatchedStatus status;
    
    // Publisher connects
    status.current_count_change = 1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
    
    // Publisher disconnects
    status.current_count_change = -1;
    status.current_count = 0;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
    
    // Publisher reconnects
    status.current_count_change = 1;
    status.current_count = 1;
    EXPECT_NO_THROW(endpoint_->on_subscription_matched(nullptr, status));
}

TEST_F(DDSMessageEndpointTest, SetTopic_SpecialCharacters) {
    // Test topic names with special characters
    endpoint_->SetTopic("/test/topic/with/slashes");
    EXPECT_EQ(endpoint_->GetTopic(), "/test/topic/with/slashes");
    
    endpoint_->SetTopic("topic_with_underscores");
    EXPECT_EQ(endpoint_->GetTopic(), "topic_with_underscores");
    
    endpoint_->SetTopic("topic-with-dashes");
    EXPECT_EQ(endpoint_->GetTopic(), "topic-with-dashes");
}

TEST_F(DDSMessageEndpointTest, SetTypeName_SpecialCharacters) {
    // Test type names with special characters
    endpoint_->SetTypeName("namespace::TypeName");
    EXPECT_EQ(endpoint_->GetTypeName(), "namespace::TypeName");
    
    endpoint_->SetTypeName("Type_With_Underscores");
    EXPECT_EQ(endpoint_->GetTypeName(), "Type_With_Underscores");
}

TEST_F(DDSMessageEndpointTest, GetTypeSupport_SetAndClear) {
    // Test setting and clearing type support
    auto ts1 = std::make_shared<TypeSupport>();
    endpoint_->SetTypeSupport(ts1);
    EXPECT_EQ(endpoint_->GetTypeSupport(), ts1);
    
    // Clear by setting to nullptr
    endpoint_->SetTypeSupport(nullptr);
    EXPECT_EQ(endpoint_->GetTypeSupport(), nullptr);
    
    // Set again
    auto ts2 = std::make_shared<TypeSupport>();
    endpoint_->SetTypeSupport(ts2);
    EXPECT_EQ(endpoint_->GetTypeSupport(), ts2);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
