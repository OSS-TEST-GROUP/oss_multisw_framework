#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "DDSProvider.h"
#include "ThreadSafeQueue.h"
#include "Types.h"
#include "VehicleDataPubSubTypes.hpp"
#include "GnssInsOrientationStampedPubSubTypes.hpp"
#include "TrajectoryPubSubTypes.hpp"
#include "GnssInsOrientationStamped.hpp"
#include "Trajectory.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <thread>
#include <chrono>

// This allows us to access private members of DDSProvider for testing
#define private public
#include "DDSProvider.h"
#undef private

using namespace eprosima::fastdds::dds;
using namespace ::testing;

// Mock DataReader for listener unit tests
//class MockDataReader : public DataReader {
//public:
//    MockDataReader() : DataReader(nullptr, nullptr, nullptr, nullptr, nullptr) {}
//    MOCK_METHOD(ReturnCode_t, take_next_sample, (void* data, SampleInfo* info), (override));
//    MOCK_METHOD(const TopicDescription*, get_topicdescription, (), (const, override));
//};


// Listener for receiving data published by DDSProvider in tests
class TestVehicleDataListener : public DataReaderListener {
public:
    std::atomic<bool> received{false};
    VehicleDataTopic sample;
    std::mutex mtx;
    std::condition_variable cv;

    void on_data_available(DataReader* reader) override {
        SampleInfo info;
        if (reader->take_next_sample(&sample, &info) == RETCODE_OK && info.valid_data) {
            std::lock_guard<std::mutex> lock(mtx);
            received = true;
            cv.notify_one();
        }
    }
};


class DDSProviderIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set a specific domain ID for testing to avoid conflicts
        setenv("VISS_DDS_DOMAIN_ID", "42", 1);

        // 1. Initialize DDSProvider
        provider = std::make_unique<DDSProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        provider->setDataChannel(data_channel);
        provider->start();


        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        pqos.name("VISS_Provider");
        // 2. Initialize DDS entities for testing
        participant = DomainParticipantFactory::get_instance()->create_participant(42, pqos, nullptr, eprosima::fastdds::dds::StatusMask::none());
        ASSERT_NE(participant, nullptr);

        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(publisher, nullptr);
        
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        ASSERT_NE(subscriber, nullptr);

        // 3. Register types
        vehicle_data_type.register_type(participant);
        gnss_type.register_type(participant);
        trajectory_type.register_type(participant);

        // 4. Create Topics
        vehicle_data_topic = participant->create_topic("VehicleData", vehicle_data_type.get_type_name(), TOPIC_QOS_DEFAULT);
        set_vehicle_data_topic = participant->create_topic("SetVehicleData", vehicle_data_type.get_type_name(), TOPIC_QOS_DEFAULT);
        gnss_topic = participant->create_topic("rt/autoware_orientation", gnss_type.get_type_name(), TOPIC_QOS_DEFAULT);
        trajectory_topic = participant->create_topic("rt/trajectory", trajectory_type.get_type_name(), TOPIC_QOS_DEFAULT);
        
        // 5. Create DataWriters for publishing test data to the provider
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        vehicle_data_writer = publisher->create_datawriter(vehicle_data_topic, writer_qos);
        gnss_writer = publisher->create_datawriter(gnss_topic, writer_qos);
        trajectory_writer = publisher->create_datawriter(trajectory_topic, writer_qos);

        // 6. Create DataReader for receiving data from the provider
        DataReaderQos reader_qos;
        reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        test_vehicle_data_reader = subscriber->create_datareader(set_vehicle_data_topic, reader_qos, &test_listener);

        ASSERT_NE(vehicle_data_writer, nullptr);
        ASSERT_NE(gnss_writer, nullptr);
        ASSERT_NE(trajectory_writer, nullptr);
        ASSERT_NE(test_vehicle_data_reader, nullptr);

        // Allow some time for discovery
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override {
        provider->stop();

        // Unset the environment variable to not affect other tests
        unsetenv("VISS_DDS_DOMAIN_ID");

        if (participant != nullptr) {
            if (publisher != nullptr) {
                publisher->delete_datawriter(vehicle_data_writer);
                publisher->delete_datawriter(gnss_writer);
                publisher->delete_datawriter(trajectory_writer);
            }
            if (subscriber != nullptr) {
                if (test_vehicle_data_reader != nullptr) {
                    subscriber->delete_datareader(test_vehicle_data_reader);
                }
            }

            if (publisher != nullptr) {
                participant->delete_publisher(publisher);
            }
            if (subscriber != nullptr) {
                participant->delete_subscriber(subscriber);
            }

            if (vehicle_data_topic != nullptr) {
                participant->delete_topic(vehicle_data_topic);
            }

            if (set_vehicle_data_topic != nullptr) {
                participant->delete_topic(set_vehicle_data_topic);
            }

            if (gnss_topic != nullptr) {
                participant->delete_topic(gnss_topic);
            }
            if (trajectory_topic != nullptr) {
                participant->delete_topic(trajectory_topic);
            }
            DomainParticipantFactory::get_instance()->delete_participant(participant);
        }
    }

    std::unique_ptr<DDSProvider> provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;

    DomainParticipant* participant = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    
    Topic* vehicle_data_topic = nullptr;
    Topic* set_vehicle_data_topic = nullptr;
    Topic* gnss_topic = nullptr;
    Topic* trajectory_topic = nullptr;

    DataWriter* vehicle_data_writer = nullptr;
    DataWriter* gnss_writer = nullptr;
    DataWriter* trajectory_writer = nullptr;
    
    DataReader* test_vehicle_data_reader = nullptr;

    TypeSupport vehicle_data_type{new VehicleDataTopicPubSubType()};
    TypeSupport gnss_type{new autoware_sensing_msgs::msg::GnssInsOrientationStampedPubSubType()};
    TypeSupport trajectory_type{new autoware_planning_msgs::msg::TrajectoryPubSubType()};
    
    TestVehicleDataListener test_listener;
};

#if 0
// --- Listener Unit Tests ---

class DDSProviderListenerTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        // The listener object is created inside each test where it's needed
    }

    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(DDSProviderListenerTest, OnDataAvailable_NoData)
{
    // 1. Setup
    DDSProvider::VehicleDataListener listener(data_channel.get());
    MockDataReader mock_reader;

    // 2. Expectation: take_next_sample will be called and return NO_DATA
    EXPECT_CALL(mock_reader, take_next_sample(_, _))
        .WillOnce(Return(RETCODE_NO_DATA));

    // 3. Action
    listener.on_data_available(&mock_reader);

    // 4. Verification: The channel should be empty
    EXPECT_TRUE(data_channel->empty());
}

TEST_F(DDSProviderListenerTest, OnDataAvailable_TakeFails)
{
    // 1. Setup
    DDSProvider::VehicleDataListener listener(data_channel.get());
    MockDataReader mock_reader;

    // 2. Expectation: take_next_sample will be called and return an error
    EXPECT_CALL(mock_reader, take_next_sample(_, _))
        .WillOnce(Return(RETCODE_ERROR));

    // 3. Action
    listener.on_data_available(&mock_reader);

    // 4. Verification: The channel should be empty
    EXPECT_TRUE(data_channel->empty());
}
#endif

// --- Integration Tests ---

TEST_F(DDSProviderIntegrationTest, StartAndStop) {
    // The setup and teardown cover the basic lifecycle.
    // If the test runs without crashing, this is a pass.
    SUCCEED();
}

TEST_F(DDSProviderIntegrationTest, ReceivesVehicleDataTopic) {
    // 1. Wait until the test writer has matched the provider's reader
    // PublicationMatchedStatus matched_status;
    // for (int i = 0; i < 20; ++i) { // Wait up to 2 seconds
    //     vehicle_data_writer->get_publication_matched_status(matched_status);
    //     if (matched_status.current_count > 0) break;
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
    // ASSERT_GT(matched_status.current_count, 0);

    // 2. Publish a sample on VehicleDataTopic
    VehicleDataTopic sample;
    sample.path("Vehicle.Speed");
    sample.value("123.45");
    ASSERT_EQ(vehicle_data_writer->write(&sample), RETCODE_OK);

    // 3. Verify the provider receives it and pushes to the channel
    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));
    
    EXPECT_EQ(received_data.path, "Vehicle.Speed");
    EXPECT_EQ(std::any_cast<std::string>(received_data.value), "123.45");
}

TEST_F(DDSProviderIntegrationTest, VehicleDataTopicNullDataChannel) {
    // 1. Reset the provider and start it without a data channel
    provider.reset();
    provider = std::make_unique<DDSProvider>();
    provider->start();

    // Give provider time to match with the writer from SetUp
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 2. Publish a sample on VehicleDataTopic
    VehicleDataTopic sample;
    sample.path("Vehicle.Speed");
    sample.value("123.45");
    ASSERT_EQ(vehicle_data_writer->write(&sample), RETCODE_OK);

    // 3. Wait a moment to ensure on_data_available would have been called
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Verify that the channel (which was not set) remains empty.
    // The original test's data_channel should be empty as it's not used by the new provider.
    ASSERT_TRUE(data_channel->empty());
}

TEST_F(DDSProviderIntegrationTest, ReceivesGnssOrientationTopic) {
    // 1. Publish a sample on the autoware orientation topic
    autoware_sensing_msgs::msg::GnssInsOrientationStamped sample;
    sample.orientation().orientation().x(0.1);
    sample.orientation().orientation().y(0.2);
    sample.orientation().orientation().z(0.3);
    sample.orientation().orientation().w(0.9);
    ASSERT_EQ(gnss_writer->write(&sample), RETCODE_OK);

    // 2. Verify the provider receives, processes, and pushes to the channel
    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));

    EXPECT_EQ(received_data.path, "Vehicle.ADS.GnssInsOrientation");
    auto doc = std::any_cast<std::shared_ptr<rapidjson::Document>>(received_data.value);
    ASSERT_TRUE(doc->IsObject());
    ASSERT_TRUE(doc->HasMember("orientation"));
    const auto& orientation = (*doc)["orientation"];
    EXPECT_STREQ(orientation["x"].GetString(), std::to_string(0.1).c_str());
    EXPECT_STREQ(orientation["w"].GetString(), std::to_string(0.9).c_str());
}

TEST_F(DDSProviderIntegrationTest, GnssOrientationTopicNullDataChannel) {
    // 1. Reset the provider and start it without a data channel
    provider.reset();
    provider = std::make_unique<DDSProvider>();
    provider->start();

    // Give provider time to match with the writer from SetUp
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    autoware_sensing_msgs::msg::GnssInsOrientationStamped sample;
    sample.orientation().orientation().x(0.1);
    sample.orientation().orientation().y(0.2);
    sample.orientation().orientation().z(0.3);
    sample.orientation().orientation().w(0.9);
    ASSERT_EQ(gnss_writer->write(&sample), RETCODE_OK);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE(data_channel->empty());
}

TEST_F(DDSProviderIntegrationTest, ReceivesTrajectoryTopic) {
    // 1. Publish a sample on the autoware trajectory topic
    autoware_planning_msgs::msg::Trajectory sample;
    autoware_planning_msgs::msg::TrajectoryPoint point;
    point.pose().position().x(1.0);
    point.longitudinal_velocity_mps(10.0f);
    sample.points().push_back(point);
    ASSERT_EQ(trajectory_writer->write(&sample), RETCODE_OK);

    // 2. Verify the provider receives, processes, and pushes to the channel
    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));

    EXPECT_EQ(received_data.path, "Vehicle.ADS.PlanningTrajectory");
    auto doc = std::any_cast<std::shared_ptr<rapidjson::Document>>(received_data.value);
    ASSERT_TRUE(doc->IsArray());
    ASSERT_EQ(doc->Size(), 1);
    const auto& received_point = (*doc)[0];
    EXPECT_STREQ(received_point["longitudinal_velocity_mps"].GetString(), std::to_string(10.0f).c_str());
}

TEST_F(DDSProviderIntegrationTest, TrajectoryTopicNullDataChannel) {
    // 1. Reset the provider and start it without a data channel
    provider.reset();
    provider = std::make_unique<DDSProvider>();
    provider->start();

    // Give provider time to match with the writer from SetUp
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    autoware_planning_msgs::msg::Trajectory sample;
    autoware_planning_msgs::msg::TrajectoryPoint point;
    point.pose().position().x(1.0);
    point.longitudinal_velocity_mps(10.0f);
    sample.points().push_back(point);
    ASSERT_EQ(trajectory_writer->write(&sample), RETCODE_OK);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE(data_channel->empty());
}

TEST_F(DDSProviderIntegrationTest, SetValuePublishesToVehicleDataTopic) {
    // 1. Call setValue on the provider
    std::string path = "Vehicle.Cabin.Light.IsOn";
    std::string value = "true";
    provider->setValue(path, value);

    // 2. Wait for the test listener to receive the data
    std::unique_lock<std::mutex> lock(test_listener.mtx);
    ASSERT_TRUE(test_listener.cv.wait_for(lock, std::chrono::seconds(2), [&]{ return test_listener.received.load(); }));

    // 3. Verify the received data
    EXPECT_EQ(test_listener.sample.path(), path);
    EXPECT_EQ(test_listener.sample.value(), value);
}

class DDSProviderDomainIdTest : public ::testing::Test {
protected:
    void TearDown() override {
        unsetenv("VISS_DDS_DOMAIN_ID");
    }
};

TEST_F(DDSProviderDomainIdTest, HandlesInvalidArgument) {
    // Set an invalid (non-numeric) value for the environment variable
    setenv("VISS_DDS_DOMAIN_ID", "not-a-number", 1);

    auto provider = std::make_unique<DDSProvider>();
    //auto data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    //auto provider->setDataChannel(data_channel);
    
    // start() should catch the std::invalid_argument, log an error, and proceed
    // with the default domain ID without crashing.
    ASSERT_NO_THROW(provider->start());
}

TEST_F(DDSProviderDomainIdTest, HandlesOutOfRange) {
    // Set a value that is too large for an integer
    setenv("VISS_DDS_DOMAIN_ID", "99999999999999999999", 1);

    auto provider = std::make_unique<DDSProvider>();
    //auto data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
    //provider->setDataChannel(data_channel);
    
    // start() should catch the std::invalid_argument, log an error, and proceed
    // with the default domain ID without crashing.
    ASSERT_NO_THROW(provider->start());
}
