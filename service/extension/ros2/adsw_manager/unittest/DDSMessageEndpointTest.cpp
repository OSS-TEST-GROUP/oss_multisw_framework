// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "adsw_manager/transmitter/DDSMessageEndpoint.hpp"



using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;


class DDSMessageEndpointTest : public ::testing::Test {
protected:
    ros2::adsw_manager::DDSMessageEndpoint endpoint;
    MockParticipant participant;
    MockPublisher publisher;
    MockSubscriber subscriber;
    MockTypeSupport type_support;
};

TEST_F(DDSMessageEndpointTest, InitializeAndFinalize) {
    endpoint.type_support_ = std::make_shared<MockTypeSupport>();
    EXPECT_TRUE(endpoint.Initialize(reinterpret_cast<DomainParticipant*>(&participant),
                                   reinterpret_cast<Publisher*>(&publisher),
                                   reinterpret_cast<Subscriber*>(&subscriber)));
    endpoint.Finalize();
}

TEST_F(DDSMessageEndpointTest, WriteMessage) {
    endpoint.data_writer_ = reinterpret_cast<MockDataWriter*>(new MockDataWriter());
    EXPECT_TRUE(endpoint.Write(reinterpret_cast<void*>(0x1234)));
    delete reinterpret_cast<MockDataWriter*>(endpoint.data_writer_);
    endpoint.data_writer_ = nullptr;
}

TEST_F(DDSMessageEndpointTest, GetPendingReadData) {
    endpoint.data_reader_ = reinterpret_cast<MockDataReader*>(new MockDataReader());
    EXPECT_EQ(endpoint.GetPendingReadData(), 42);
    delete reinterpret_cast<MockDataReader*>(endpoint.data_reader_);
    endpoint.data_reader_ = nullptr;
}

TEST_F(DDSMessageEndpointTest, GetPendingWriteData) {
    EXPECT_EQ(endpoint.GetPendingWriteData(), 0);
}


TEST_F(DDSMessageEndpointTest, OnDataAvailable) {
    // Simulate data available
    endpoint.data_reader_ = reinterpret_cast<MockDataReader*>(new MockDataReader());
    EXPECT_NO_THROW(endpoint.OnDataAvailable());
    delete reinterpret_cast<MockDataReader*>(endpoint.data_reader_);
    endpoint.data_reader_ = nullptr;
}

TEST_F(DDSMessageEndpointTest, OnSubscriptionMatched) {
    // Simulate subscription matched
    endpoint.data_reader_ = reinterpret_cast<MockDataReader*>(new MockDataReader());
    SubscriptionMatchedStatus status;
    EXPECT_NO_THROW(endpoint.on_subscription_matched(endpoint.data_reader_, status));
    delete reinterpret_cast<MockDataReader*>(endpoint.data_reader_);
    endpoint.data_reader_ = nullptr;
}

TEST_F(DDSMessageEndpointTest, OnPublicationMatched) {
    // Simulate publication matched
    endpoint.data_writer_ = reinterpret_cast<MockDataWriter*>(new MockDataWriter());
    PublicationMatchedStatus status;
    status.current_count_change = 1; // Simulate a match
    EXPECT_NO_THROW(endpoint.on_publication_matched(endpoint.data_writer_, status));
    delete reinterpret_cast<MockDataWriter*>(endpoint.data_writer_);
    endpoint.data_writer_ = nullptr;
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
