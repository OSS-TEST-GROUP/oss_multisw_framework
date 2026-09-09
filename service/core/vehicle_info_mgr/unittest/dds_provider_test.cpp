#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Types.h"
#include "ThreadSafeQueue.h"
#include "VssTree.h"

// Access private members of DDSProvider for direct unit testing of all handlers
#define private public
#include "DDSProvider.h"
#undef private

#include "VehicleDataPubSubTypes.hpp"
#include "GnssInsOrientationStampedPubSubTypes.hpp"
#include "GnssInsOrientationStamped.hpp"
#include "autoware_planning_msgs/msg/TrajectoryPubSubTypes.hpp"
#include "autoware_planning_msgs/msg/Trajectory.hpp"
#include "autoware_planning_msgs/msg/PathPubSubTypes.hpp"
#include "autoware_planning_msgs/msg/Path.hpp"
#include "autoware_adapi_v1_msgs/msg/VehicleKinematicsPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/VehicleKinematics.hpp"
#include "autoware_adapi_v1_msgs/msg/VehicleStatusPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/VehicleStatus.hpp"
#include "autoware_adapi_v1_msgs/msg/RouteStatePubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/RouteState.hpp"
#include "autoware_adapi_v1_msgs/msg/RoutePubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/Route.hpp"
#include "autoware_adapi_v1_msgs/msg/MrmStatePubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/MrmState.hpp"
#include "autoware_adapi_v1_msgs/msg/DynamicObjectArrayPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/DynamicObjectArray.hpp"
#include "tier4_external_api_msgs/msg/EmergencyPubSubTypes.hpp"
#include "tier4_external_api_msgs/msg/Emergency.hpp"
#include "autoware_system_msgs/msg/HazardStatusStampedPubSubTypes.hpp"
#include "autoware_system_msgs/msg/HazardStatusStamped.hpp"
#include "tier4_system_msgs/msg/OperationModeAvailabilityPubSubTypes.hpp"
#include "tier4_system_msgs/msg/OperationModeAvailability.hpp"
#include "autoware_adapi_v1_msgs/msg/OperationModeStatePubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/OperationModeState.hpp"
#include "autoware_adapi_v1_msgs/msg/VelocityFactorArrayPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/VelocityFactorArray.hpp"
#include "autoware_adapi_v1_msgs/msg/SteeringFactorArrayPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/SteeringFactorArray.hpp"
#include "autoware_adapi_v1_msgs/msg/DiagGraphStatusPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/DiagGraphStatus.hpp"
#include "autoware_adapi_v1_msgs/msg/DiagGraphStructPubSubTypes.hpp"
#include "autoware_adapi_v1_msgs/msg/DiagGraphStruct.hpp"
#include "autoware_perception_msgs/msg/DetectedObjectsPubSubTypes.hpp"
#include "autoware_perception_msgs/msg/DetectedObjects.hpp"
#include "autoware_perception_msgs/msg/TrafficLightGroupArrayPubSubTypes.hpp"
#include "autoware_perception_msgs/msg/TrafficLightGroupArray.hpp"

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

using namespace eprosima::fastdds::dds;
using namespace ::testing;

// =========================================================================
// Basic & Lifecycle Tests
// =========================================================================

class DDSProviderLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("VISS_DDS_DOMAIN_ID", "42", 1);
        provider = std::make_unique<DDSProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        provider->setDataChannel(data_channel);
    }

    void TearDown() override {
        provider->stop();
        unsetenv("VISS_DDS_DOMAIN_ID");
    }

    std::unique_ptr<DDSProvider> provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
};

TEST_F(DDSProviderLifecycleTest, StartAndStopLifecycle) {
    EXPECT_EQ(provider->getName(), "DDSProvider");
    provider->start();
    EXPECT_TRUE(provider->active_);

    // Multiple start() is idempotent
    provider->start();
    EXPECT_TRUE(provider->active_);

    provider->stop();
    EXPECT_FALSE(provider->active_);

    // Multiple stop() is idempotent
    provider->stop();
    EXPECT_FALSE(provider->active_);
}

TEST_F(DDSProviderLifecycleTest, SetVssTreeAndValue) {
    const char* config_path = std::getenv("VEHICLE_CONFIG_PATH");
    std::string him_path = (config_path ? std::string(config_path) : "../config") + "/viss.him";
    auto vss_tree = std::make_shared<viss::VSSTree>(him_path);
    provider->setVssTree(vss_tree);
    EXPECT_EQ(provider->vss_tree_, vss_tree);

    // setValue when writer is null is safe no-op
    provider->vehicle_data_writer_ = nullptr;
    EXPECT_NO_THROW(provider->setValue("Vehicle.Speed", std::string("100")));
}

TEST_F(DDSProviderLifecycleTest, PushToDataChannelEmptyChannel) {
    provider->setDataChannel(nullptr);
    // Should not crash when channel is null
    EXPECT_NO_THROW(provider->pushToDataChannel("Vehicle.Speed", std::string("100"), 0));
}

// =========================================================================
// Domain ID Configuration Tests
// =========================================================================

TEST(DDSProviderDomainIdTest, HandlesInvalidAndOutOfRange) {
    unsetenv("DDS_DOMAIN_ID"); // keep the per-service override the only input
    setenv("VISS_DDS_DOMAIN_ID", "not-a-number", 1);
    auto p1 = std::make_unique<DDSProvider>();
    EXPECT_NO_THROW(p1->start());
    p1->stop();

    setenv("VISS_DDS_DOMAIN_ID", "99999999999999999999", 1);
    auto p2 = std::make_unique<DDSProvider>();
    EXPECT_NO_THROW(p2->start());
    p2->stop();

    unsetenv("VISS_DDS_DOMAIN_ID");
}

// DDS 평면 정규 키 폴백: hmi_launcher Settings 는 DDS_DOMAIN_ID 를 domain.env 에
// 쓴다. VISS 전용 override 가 없으면 이 값을 따라가야 보드 전체가 한 DDS 평면에
// 모인다 (ROS_DOMAIN_ID 는 별개 평면이라 여기서 보지 않는다).
TEST(DDSProviderDomainIdTest, FallsBackToCanonicalDdsDomainId) {
    unsetenv("VISS_DDS_DOMAIN_ID");
    setenv("DDS_DOMAIN_ID", "89", 1);
    auto p = std::make_unique<DDSProvider>();
    EXPECT_NO_THROW(p->start());
    p->stop();
    unsetenv("DDS_DOMAIN_ID");
}

// =========================================================================
// Autoware Integration Tests (Publish via FastDDS -> Provider Receives)
// =========================================================================

class DDSProviderAutowareIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("VISS_DDS_DOMAIN_ID", "89", 1);

        provider = std::make_unique<DDSProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        provider->setDataChannel(data_channel);
        provider->start();

        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        pqos.name("VISS_Test_Publisher");
        participant = DomainParticipantFactory::get_instance()->create_participant(89, pqos, nullptr, StatusMask::none());
        ASSERT_NE(participant, nullptr);

        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(publisher, nullptr);

        // Register Types
        gnss_type.register_type(participant);
        trajectory_type.register_type(participant);

        // Create Topics matching DDSProvider
        gnss_topic = participant->create_topic("rt/autoware_orientation", gnss_type.get_type_name(), TOPIC_QOS_DEFAULT);
        trajectory_topic = participant->create_topic("rt/planning/scenario_planning/trajectory", trajectory_type.get_type_name(), TOPIC_QOS_DEFAULT);

        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        gnss_writer = publisher->create_datawriter(gnss_topic, writer_qos);
        trajectory_writer = publisher->create_datawriter(trajectory_topic, writer_qos);

        ASSERT_NE(gnss_writer, nullptr);
        ASSERT_NE(trajectory_writer, nullptr);

        // Allow discovery
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    void TearDown() override {
        provider->stop();
        unsetenv("VISS_DDS_DOMAIN_ID");

        if (participant != nullptr) {
            participant->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            participant = nullptr;
        }
    }

    std::unique_ptr<DDSProvider> provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;

    DomainParticipant* participant = nullptr;
    Publisher* publisher = nullptr;

    Topic* gnss_topic = nullptr;
    Topic* trajectory_topic = nullptr;

    DataWriter* gnss_writer = nullptr;
    DataWriter* trajectory_writer = nullptr;

    TypeSupport gnss_type{new autoware_sensing_msgs::msg::GnssInsOrientationStampedPubSubType()};
    TypeSupport trajectory_type{new autoware_planning_msgs::msg::TrajectoryPubSubType()};
};

TEST_F(DDSProviderAutowareIntegrationTest, ReceivesGnssOrientationTopic) {
    autoware_sensing_msgs::msg::GnssInsOrientationStamped sample;
    sample.orientation().orientation().x(0.1);
    sample.orientation().orientation().y(0.2);
    sample.orientation().orientation().z(0.3);
    sample.orientation().orientation().w(0.9);
    ASSERT_EQ(gnss_writer->write(&sample), RETCODE_OK);

    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));
    EXPECT_EQ(received_data.path, "Vehicle.ADS.GnssInsOrientation");
}

TEST_F(DDSProviderAutowareIntegrationTest, ReceivesTrajectoryTopic) {
    autoware_planning_msgs::msg::Trajectory sample;
    autoware_planning_msgs::msg::TrajectoryPoint point;
    point.pose().position().x(1.0);
    point.longitudinal_velocity_mps(10.0f);
    sample.points().push_back(point);
    ASSERT_EQ(trajectory_writer->write(&sample), RETCODE_OK);

    VehicleData received_data;
    ASSERT_TRUE(data_channel->wait_and_pop(received_data));
    EXPECT_EQ(received_data.path, "Vehicle.ADS.Planning.Trajectory.PointCount");
}

// =========================================================================
// Comprehensive Message Handlers Unit & Branch Coverage Tests
// =========================================================================

class DDSProviderHandlersTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("VISS_DDS_DOMAIN_ID", "89", 1);
        provider = std::make_unique<DDSProvider>();
        data_channel = std::make_shared<ThreadSafeQueue<VehicleData>>();
        provider->setDataChannel(data_channel);
        provider->start();

        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        pqos.name("VISS_Handlers_Test");
        participant = DomainParticipantFactory::get_instance()->create_participant(89, pqos, nullptr, StatusMask::none());
        ASSERT_NE(participant, nullptr);
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        ASSERT_NE(publisher, nullptr);
    }

    void TearDown() override {
        provider->stop();
        unsetenv("VISS_DDS_DOMAIN_ID");
        if (participant != nullptr) {
            participant->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            participant = nullptr;
        }
        writers_map.clear();
        topics_map.clear();
    }

    template<typename T, typename TPubSub>
    void publishSample(const std::string& topic_name, T& sample, bool best_effort = false) {
        DataWriter* writer = nullptr;
        auto it = writers_map.find(topic_name);
        if (it != writers_map.end()) {
            writer = it->second;
        } else {
            TypeSupport ts{new TPubSub()};
            ts.register_type(participant);
            Topic* topic = participant->create_topic(topic_name, ts.get_type_name(), TOPIC_QOS_DEFAULT);
            DataWriterQos qos = DATAWRITER_QOS_DEFAULT;
            if (best_effort) {
                qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
            } else {
                qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            }
            writer = publisher->create_datawriter(topic, qos);
            topics_map[topic_name] = topic;
            writers_map[topic_name] = writer;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        ASSERT_NE(writer, nullptr);
        ASSERT_EQ(writer->write(&sample), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    std::unique_ptr<DDSProvider> provider;
    std::shared_ptr<ThreadSafeQueue<VehicleData>> data_channel;
    DomainParticipant* participant = nullptr;
    Publisher* publisher = nullptr;
    std::map<std::string, Topic*> topics_map;
    std::map<std::string, DataWriter*> writers_map;
};

// 1. VehicleStatus Handler (all gears and turn signals)
TEST_F(DDSProviderHandlersTest, VehicleStatus_AllBranches) {
    // Test DRIVE + LEFT
    {
        autoware_adapi_v1_msgs::msg::VehicleStatus sample;
        sample.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::DRIVE);
        sample.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::LEFT);
        sample.hazard_lights().status(autoware_adapi_v1_msgs::msg::HazardLights_Constants::ENABLE);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", sample, true);

        VehicleData vd;
        bool foundGear = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.Powertrain.Transmission.SelectedGear") {
                EXPECT_EQ(std::any_cast<std::string>(vd.value), "127");
                foundGear = true;
            }
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundGear);
    }

    // Test REVERSE + RIGHT
    {
        autoware_adapi_v1_msgs::msg::VehicleStatus sample;
        sample.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::REVERSE);
        sample.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::RIGHT);
        sample.hazard_lights().status(autoware_adapi_v1_msgs::msg::HazardLights_Constants::DISABLE);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", sample, true);

        VehicleData vd;
        bool foundGear = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.Powertrain.Transmission.SelectedGear") {
                EXPECT_EQ(std::any_cast<std::string>(vd.value), "-1");
                foundGear = true;
            }
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundGear);
    }

    // Test PARK, LOW, NEUTRAL, DISABLE turn signals
    {
        autoware_adapi_v1_msgs::msg::VehicleStatus sample;
        sample.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::PARK);
        sample.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::DISABLE);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", sample, true);

        sample.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::LOW);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", sample, true);

        sample.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::NEUTRAL);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", sample, true);
    }
}

// 2. VehicleKinematics Handler
TEST_F(DDSProviderHandlersTest, VehicleKinematics_FullParsing) {
    autoware_adapi_v1_msgs::msg::VehicleKinematics sample;
    sample.geographic_pose().position().latitude(37.5665);
    sample.geographic_pose().position().longitude(126.9780);
    sample.geographic_pose().position().altitude(50.0);
    sample.pose().pose_().pose_().orientation().w(1.0);
    sample.twist().twist_().twist_().linear().x(20.0); // 72 km/h
    sample.accel().accel_().accel_().linear().x(1.5);

    std::array<double, 36> cov{};
    cov.fill(0.01);
    sample.pose().pose_().covariance(cov);
    sample.twist().twist_().covariance(cov);
    sample.accel().accel_().covariance(cov);

    publishSample<autoware_adapi_v1_msgs::msg::VehicleKinematics, autoware_adapi_v1_msgs::msg::VehicleKinematicsPubSubType>("rt/api/vehicle/kinematics", sample, true);

    VehicleData vd;
    bool foundLat = false, foundSpeed = false;
    while (data_channel->wait_and_pop(vd)) {
        if (vd.path == "Vehicle.CurrentLocation.Latitude") foundLat = true;
        if (vd.path == "Vehicle.Speed") foundSpeed = true;
        if (data_channel->empty()) break;
    }
    EXPECT_TRUE(foundLat);
    EXPECT_TRUE(foundSpeed);
}

// 3. Emergency & HazardStatus
TEST_F(DDSProviderHandlersTest, EmergencyAndHazardStatus_AllLevels) {
    // Emergency
    {
        tier4_external_api_msgs::msg::Emergency emergency_msg;
        emergency_msg.emergency(true);
        publishSample<tier4_external_api_msgs::msg::Emergency, tier4_external_api_msgs::msg::EmergencyPubSubType>("rt/api/autoware/get/emergency", emergency_msg, false);

        VehicleData vd;
        bool foundEmer = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.IsEmergency") {
                foundEmer = true;
            }
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundEmer);
    }

    // HazardStatus: NO_FAULT, SAFE_FAULT, LATENT_FAULT, SINGLE_POINT_FAULT
    {
        autoware_system_msgs::msg::HazardStatusStamped hs;
        hs.status().level(autoware_system_msgs::msg::HazardStatus_Constants::NO_FAULT);
        hs.status().emergency(false);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hs, false);

        hs.status().level(autoware_system_msgs::msg::HazardStatus_Constants::SAFE_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hs, false);

        hs.status().level(autoware_system_msgs::msg::HazardStatus_Constants::LATENT_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hs, false);

        hs.status().level(autoware_system_msgs::msg::HazardStatus_Constants::SINGLE_POINT_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hs, false);

        VehicleData vd;
        bool foundHazard = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.Safety.HazardStatus.Level") {
                foundHazard = true;
            }
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundHazard);
    }
}

// 4. OperationMode State & Availability
TEST_F(DDSProviderHandlersTest, OperationMode_StateAndAvailability) {
    // OperationModeState (STOP, AUTONOMOUS, LOCAL, REMOTE)
    {
        autoware_adapi_v1_msgs::msg::OperationModeState op_state;
        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::AUTONOMOUS);
        op_state.is_autoware_control_enabled(true);
        op_state.is_in_transition(false);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::STOP);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::LOCAL_);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::REMOTE);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        VehicleData vd;
        bool foundMode = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.OperationMode.State.Mode") foundMode = true;
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundMode);
    }

    // Availability
    {
        tier4_system_msgs::msg::OperationModeAvailability avail;
        avail.autonomous(true);
        avail.stop(true);
        avail.is_local(false);
        avail.remote(false);
        avail.emergency_stop(false);
        avail.comfortable_stop(false);
        avail.pull_over(true);
        publishSample<tier4_system_msgs::msg::OperationModeAvailability, tier4_system_msgs::msg::OperationModeAvailabilityPubSubType>("rt/system/operation_mode/availability", avail, false);

        VehicleData vd;
        bool foundAvail = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.OperationMode.Availability.IsAutonomous") foundAvail = true;
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundAvail);
    }
}

// 5. DynamicObjectArray & Classifications
TEST_F(DDSProviderHandlersTest, DynamicObjectArray_AllClassifications) {
    autoware_adapi_v1_msgs::msg::DynamicObjectArray obj_array;
    
    auto addObj = [&](uint8_t label) {
        autoware_adapi_v1_msgs::msg::DynamicObject obj;
        std::array<uint8_t, 16> id = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        obj.id().uuid(id);
        obj.existence_probability(0.95);
        autoware_adapi_v1_msgs::msg::ObjectClassification cls;
        cls.label(label);
        cls.probability(0.99);
        obj.classification().push_back(cls);
        obj_array.objects().push_back(obj);
    };

    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::CAR);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRUCK);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BUS);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRAILER);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::MOTORCYCLE);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BICYCLE);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::PEDESTRIAN);
    addObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::UNKNOWN);

    publishSample<autoware_adapi_v1_msgs::msg::DynamicObjectArray, autoware_adapi_v1_msgs::msg::DynamicObjectArrayPubSubType>("rt/api/perception/objects", obj_array, false);

    VehicleData vd;
    bool foundObjects = false;
    while (data_channel->wait_and_pop(vd)) {
        if (vd.path == "Vehicle.ADS.Perception.DynamicObject.Objects") foundObjects = true;
        if (data_channel->empty()) break;
    }
    EXPECT_TRUE(foundObjects);
}

// 6. VelocityFactorArray & SteeringFactorArray
TEST_F(DDSProviderHandlersTest, Factors_AllStatuses) {
    // VelocityFactor
    {
        autoware_adapi_v1_msgs::msg::VelocityFactorArray v_arr;
        autoware_adapi_v1_msgs::msg::VelocityFactor f1;
        f1.status(autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::APPROACHING);
        f1.behavior("Intersection");
        f1.distance(15.0f);
        v_arr.factors().push_back(f1);

        autoware_adapi_v1_msgs::msg::VelocityFactor f2;
        f2.status(autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::STOPPED);
        v_arr.factors().push_back(f2);

        publishSample<autoware_adapi_v1_msgs::msg::VelocityFactorArray, autoware_adapi_v1_msgs::msg::VelocityFactorArrayPubSubType>("rt/api/planning/velocity_factors", v_arr, false);

        VehicleData vd;
        bool foundVFactor = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.Planning.VelocityFactor.Factors") foundVFactor = true;
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundVFactor);
    }

    // SteeringFactor
    {
        autoware_adapi_v1_msgs::msg::SteeringFactorArray s_arr;
        autoware_adapi_v1_msgs::msg::SteeringFactor sf1;
        sf1.direction(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::LEFT);
        sf1.status(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::TURNING);
        s_arr.factors().push_back(sf1);

        autoware_adapi_v1_msgs::msg::SteeringFactor sf2;
        sf2.direction(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::RIGHT);
        sf2.status(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::APPROACHING);
        s_arr.factors().push_back(sf2);

        autoware_adapi_v1_msgs::msg::SteeringFactor sf3;
        sf3.direction(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::STRAIGHT);
        sf3.status(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::UNKNOWN);
        s_arr.factors().push_back(sf3);

        autoware_adapi_v1_msgs::msg::SteeringFactor sf4;
        sf4.direction(autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::UNKNOWN);
        sf4.status(999);
        s_arr.factors().push_back(sf4);

        publishSample<autoware_adapi_v1_msgs::msg::SteeringFactorArray, autoware_adapi_v1_msgs::msg::SteeringFactorArrayPubSubType>("rt/api/planning/steering_factors", s_arr, false);

        VehicleData vd;
        bool foundSFactor = false;
        while (data_channel->wait_and_pop(vd)) {
            if (vd.path == "Vehicle.ADS.Planning.SteeringFactor.Factors") foundSFactor = true;
            if (data_channel->empty()) break;
        }
        EXPECT_TRUE(foundSFactor);
    }

    // VelocityFactorArray all statuses and cooperation decisions
    {
        autoware_adapi_v1_msgs::msg::VelocityFactorArray vf_arr;
        autoware_adapi_v1_msgs::msg::VelocityFactor vf1, vf2, vf3;
        vf1.status(autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::APPROACHING);
        vf2.status(autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::STOPPED);
        vf3.status(autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::UNKNOWN);

        auto addCoop = [&](autoware_adapi_v1_msgs::msg::VelocityFactor& vf, uint8_t auto_dec, uint8_t coop_dec) {
            autoware_adapi_v1_msgs::msg::CooperationStatus cs;
            cs.autonomous().decision(auto_dec);
            cs.cooperator().decision(coop_dec);
            cs.cancellable(true);
            vf.cooperation().push_back(cs);
        };

        addCoop(vf1, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::DEACTIVATE, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::DEACTIVATE);
        addCoop(vf1, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::ACTIVATE, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::ACTIVATE);
        addCoop(vf2, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::AUTONOMOUS, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::AUTONOMOUS);
        addCoop(vf2, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNDECIDED, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNDECIDED);
        addCoop(vf3, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNKNOWN, autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNKNOWN);

        vf_arr.factors().push_back(vf1);
        vf_arr.factors().push_back(vf2);
        vf_arr.factors().push_back(vf3);
        publishSample<autoware_adapi_v1_msgs::msg::VelocityFactorArray, autoware_adapi_v1_msgs::msg::VelocityFactorArrayPubSubType>("rt/api/planning/velocity_factors", vf_arr, false);
    }

    // GnssInsOrientationStamped
    {
        autoware_sensing_msgs::msg::GnssInsOrientationStamped gnss_sample;
        gnss_sample.orientation().orientation().x(0.1);
        gnss_sample.orientation().orientation().y(0.2);
        gnss_sample.orientation().orientation().z(0.3);
        gnss_sample.orientation().orientation().w(0.9);
        gnss_sample.orientation().rmse_rotation_x(0.01);
        gnss_sample.orientation().rmse_rotation_y(0.01);
        gnss_sample.orientation().rmse_rotation_z(0.01);
        publishSample<autoware_sensing_msgs::msg::GnssInsOrientationStamped, autoware_sensing_msgs::msg::GnssInsOrientationStampedPubSubType>("rt/autoware_orientation", gnss_sample, false);
    }
}

// 7. RouteState, Route & MrmState
TEST_F(DDSProviderHandlersTest, RoutingAndMrmState_AllBranches) {
    // RouteState (all states: UNSET, SETUP, ARRIVED, CHANGING, UNKNOWN)
    {
        autoware_adapi_v1_msgs::msg::RouteState r_state;
        r_state.state(autoware_adapi_v1_msgs::msg::RouteState_Constants::UNSET);
        publishSample<autoware_adapi_v1_msgs::msg::RouteState, autoware_adapi_v1_msgs::msg::RouteStatePubSubType>("rt/api/routing/state", r_state, false);

        r_state.state(autoware_adapi_v1_msgs::msg::RouteState_Constants::SETUP);
        publishSample<autoware_adapi_v1_msgs::msg::RouteState, autoware_adapi_v1_msgs::msg::RouteStatePubSubType>("rt/api/routing/state", r_state, false);

        r_state.state(autoware_adapi_v1_msgs::msg::RouteState_Constants::ARRIVED);
        publishSample<autoware_adapi_v1_msgs::msg::RouteState, autoware_adapi_v1_msgs::msg::RouteStatePubSubType>("rt/api/routing/state", r_state, false);

        r_state.state(autoware_adapi_v1_msgs::msg::RouteState_Constants::CHANGING);
        publishSample<autoware_adapi_v1_msgs::msg::RouteState, autoware_adapi_v1_msgs::msg::RouteStatePubSubType>("rt/api/routing/state", r_state, false);

        r_state.state(autoware_adapi_v1_msgs::msg::RouteState_Constants::UNKNOWN);
        publishSample<autoware_adapi_v1_msgs::msg::RouteState, autoware_adapi_v1_msgs::msg::RouteStatePubSubType>("rt/api/routing/state", r_state, false);
    }

    // Route with segments
    {
        autoware_adapi_v1_msgs::msg::Route route;
        autoware_adapi_v1_msgs::msg::RouteData r_data;
        autoware_adapi_v1_msgs::msg::RouteSegment seg;
        seg.preferred().id(12345);
        seg.preferred().type("LANE");
        autoware_adapi_v1_msgs::msg::RoutePrimitive alt;
        alt.id(67890);
        alt.type("SHOULDER");
        seg.alternatives().push_back(alt);
        r_data.segments().push_back(seg);
        route.data().push_back(r_data);
        publishSample<autoware_adapi_v1_msgs::msg::Route, autoware_adapi_v1_msgs::msg::RoutePubSubType>("rt/api/routing/route", route, false);
    }

    // MrmState (all states and behaviors)
    {
        autoware_adapi_v1_msgs::msg::MrmState mrm;
        mrm.state(autoware_adapi_v1_msgs::msg::MrmState_Constants::NORMAL);
        mrm.behavior(autoware_adapi_v1_msgs::msg::MrmState_Constants::NONE);
        publishSample<autoware_adapi_v1_msgs::msg::MrmState, autoware_adapi_v1_msgs::msg::MrmStatePubSubType>("rt/api/fail_safe/mrm_state", mrm, false);

        mrm.state(autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_OPERATING);
        mrm.behavior(autoware_adapi_v1_msgs::msg::MrmState_Constants::EMERGENCY_STOP);
        publishSample<autoware_adapi_v1_msgs::msg::MrmState, autoware_adapi_v1_msgs::msg::MrmStatePubSubType>("rt/api/fail_safe/mrm_state", mrm, false);

        mrm.state(autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_SUCCEEDED);
        mrm.behavior(autoware_adapi_v1_msgs::msg::MrmState_Constants::COMFORTABLE_STOP);
        publishSample<autoware_adapi_v1_msgs::msg::MrmState, autoware_adapi_v1_msgs::msg::MrmStatePubSubType>("rt/api/fail_safe/mrm_state", mrm, false);

        mrm.state(autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_FAILED);
        mrm.behavior(autoware_adapi_v1_msgs::msg::MrmState_Constants::PULL_OVER);
        publishSample<autoware_adapi_v1_msgs::msg::MrmState, autoware_adapi_v1_msgs::msg::MrmStatePubSubType>("rt/api/fail_safe/mrm_state", mrm, false);

        mrm.state(autoware_adapi_v1_msgs::msg::MrmState_Constants::UNKNOWN);
        mrm.behavior(autoware_adapi_v1_msgs::msg::MrmState_Constants::UNKNOWN);
        publishSample<autoware_adapi_v1_msgs::msg::MrmState, autoware_adapi_v1_msgs::msg::MrmStatePubSubType>("rt/api/fail_safe/mrm_state", mrm, false);
    }
}

// 8. DetectedObjects, Path, TrafficSignals, DiagGraph
TEST_F(DDSProviderHandlersTest, PerceptionAndDiagGraph_AllBranches) {
    // DetectedObjects (all classification types and shapes)
    {
        autoware_perception_msgs::msg::DetectedObjects det_objs;

        auto addObj = [&](uint8_t label_type, uint8_t shape_type) {
            autoware_perception_msgs::msg::DetectedObject obj;
            obj.existence_probability(0.95f);

            autoware_perception_msgs::msg::ObjectClassification cls;
            cls.label(label_type);
            cls.probability(0.99f);
            obj.classification().push_back(cls);

            autoware_perception_msgs::msg::Shape shp;
            shp.type(shape_type);
            shp.dimensions().x(4.5);
            shp.dimensions().y(2.0);
            shp.dimensions().z(1.5);
            geometry_msgs::msg::Point32 pt;
            pt.x(1.0f); pt.y(2.0f); pt.z(0.0f);
            shp.footprint().points().push_back(pt);
            obj.shape_(shp);

            autoware_perception_msgs::msg::DetectedObjectKinematics kin;
            kin.has_position_covariance(true);
            kin.has_twist(true);
            kin.has_twist_covariance(true);
            obj.kinematics(kin);

            det_objs.objects().push_back(obj);
        };

        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::CAR, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::TRUCK, autoware_perception_msgs::msg::Shape_Constants::CYLINDER);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::BUS, autoware_perception_msgs::msg::Shape_Constants::POLYGON);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::TRAILER, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::MOTORCYCLE, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::BICYCLE, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::PEDESTRIAN, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);
        addObj(autoware_perception_msgs::msg::ObjectClassification_Constants::UNKNOWN, autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX);

        publishSample<autoware_perception_msgs::msg::DetectedObjects, autoware_perception_msgs::msg::DetectedObjectsPubSubType>("rt/perception/object_recognition/detection/objects", det_objs, false);
    }

    // DynamicObjectArray (all classification types and shapes)
    {
        autoware_adapi_v1_msgs::msg::DynamicObjectArray dyn_objs;

        auto addDynObj = [&](uint8_t label_type, uint8_t shape_type) {
            autoware_adapi_v1_msgs::msg::DynamicObject obj;
            obj.existence_probability(0.95);

            autoware_adapi_v1_msgs::msg::ObjectClassification cls;
            cls.label(label_type);
            cls.probability(0.99);
            obj.classification().push_back(cls);

            shape_msgs::msg::SolidPrimitive shp;
            shp.type(shape_type);
            shp.dimensions().push_back(4.5);
            shp.dimensions().push_back(2.0);
            shp.dimensions().push_back(1.5);
            obj.shape(shp);

            autoware_adapi_v1_msgs::msg::DynamicObjectKinematics kin;
            obj.kinematics(kin);

            dyn_objs.objects().push_back(obj);
        };

        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::CAR, shape_msgs::msg::SolidPrimitive_Constants::BOX);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRUCK, shape_msgs::msg::SolidPrimitive_Constants::CYLINDER);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BUS, shape_msgs::msg::SolidPrimitive_Constants::SPHERE);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRAILER, shape_msgs::msg::SolidPrimitive_Constants::CONE);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::MOTORCYCLE, shape_msgs::msg::SolidPrimitive_Constants::BOX);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BICYCLE, shape_msgs::msg::SolidPrimitive_Constants::BOX);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::PEDESTRIAN, shape_msgs::msg::SolidPrimitive_Constants::BOX);
        addDynObj(autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::UNKNOWN, shape_msgs::msg::SolidPrimitive_Constants::BOX);

        publishSample<autoware_adapi_v1_msgs::msg::DynamicObjectArray, autoware_adapi_v1_msgs::msg::DynamicObjectArrayPubSubType>("rt/api/perception/objects", dyn_objs, false);
    }

    // Path
    {
        autoware_planning_msgs::msg::Path path;
        autoware_planning_msgs::msg::PathPoint pt;
        pt.longitudinal_velocity_mps(15.0f);
        path.points().push_back(pt);
        publishSample<autoware_planning_msgs::msg::Path, autoware_planning_msgs::msg::PathPubSubType>("rt/planning/scenario_planning/lane_driving/motion_planning/path_smoother/path", path, false);
    }

    // TrafficLightGroupArray
    {
        autoware_perception_msgs::msg::TrafficLightGroupArray tl_arr;
        autoware_perception_msgs::msg::TrafficLightGroup grp;
        grp.traffic_light_group_id(101);

        auto addElem = [&](uint8_t color, uint8_t shape, uint8_t status) {
            autoware_perception_msgs::msg::TrafficLightElement elem;
            elem.color(color);
            elem.shape(shape);
            elem.status(status);
            elem.confidence(0.98f);
            grp.elements().push_back(elem);
        };

        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::CIRCLE, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::AMBER, autoware_perception_msgs::msg::TrafficLightElement_Constants::LEFT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_OFF);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::GREEN, autoware_perception_msgs::msg::TrafficLightElement_Constants::RIGHT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::FLASHING);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::WHITE, autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_LEFT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_RIGHT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_LEFT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_RIGHT_ARROW, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);
        addElem(autoware_perception_msgs::msg::TrafficLightElement_Constants::RED, autoware_perception_msgs::msg::TrafficLightElement_Constants::CROSS, autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON);

        tl_arr.traffic_light_groups().push_back(grp);
        publishSample<autoware_perception_msgs::msg::TrafficLightGroupArray, autoware_perception_msgs::msg::TrafficLightGroupArrayPubSubType>("rt/perception/traffic_light_recognition/traffic_signals", tl_arr, false);
    }

    // DiagGraphStatus & DiagGraphStruct
    {
        autoware_adapi_v1_msgs::msg::DiagGraphStatus dg_status;
        dg_status.id("graph-1");
        autoware_adapi_v1_msgs::msg::DiagNodeStatus node0, node1, node2, node3;
        node0.level(0);
        node1.level(1);
        node2.level(2);
        node3.level(3);
        dg_status.nodes().push_back(node0);
        dg_status.nodes().push_back(node1);
        dg_status.nodes().push_back(node2);
        dg_status.nodes().push_back(node3);

        publishSample<autoware_adapi_v1_msgs::msg::DiagGraphStatus, autoware_adapi_v1_msgs::msg::DiagGraphStatusPubSubType>("rt/api/system/diagnostics/status", dg_status, true);

        autoware_adapi_v1_msgs::msg::DiagGraphStruct dg_struct;
        dg_struct.id("graph-1");
        autoware_adapi_v1_msgs::msg::DiagNodeStruct node;
        node.path("diag/node/sensor");
        dg_struct.nodes().push_back(node);
        publishSample<autoware_adapi_v1_msgs::msg::DiagGraphStruct, autoware_adapi_v1_msgs::msg::DiagGraphStructPubSubType>("rt/api/system/diagnostics/struct", dg_struct, true);
    }
}

// 9. HazardStatus and Emergency
TEST_F(DDSProviderHandlersTest, HazardStatusAndEmergency_AllBranches) {
    // HazardStatusStamped
    {
        autoware_system_msgs::msg::HazardStatusStamped hz_stamped;
        autoware_system_msgs::msg::HazardStatus& hz = hz_stamped.status();
        hz.level(autoware_system_msgs::msg::HazardStatus_Constants::NO_FAULT);
        hz.emergency(false);

        auto makeDiag = [](uint8_t level, const std::string& name) {
            diagnostic_msgs::msg::DiagnosticStatus ds;
            ds.name(name);
            ds.level(level);
            ds.message("message_" + name);
            ds.hardware_id("hw_" + name);
            diagnostic_msgs::msg::KeyValue kv;
            kv.key("voltage");
            kv.value("12.5");
            ds.values().push_back(kv);
            return ds;
        };

        hz.diag_no_fault().push_back(makeDiag(diagnostic_msgs::msg::DiagnosticStatus_Constants::OK, "sensor_ok"));
        hz.diag_safe_fault().push_back(makeDiag(diagnostic_msgs::msg::DiagnosticStatus_Constants::WARN, "sensor_warn"));
        hz.diag_latent_fault().push_back(makeDiag(diagnostic_msgs::msg::DiagnosticStatus_Constants::ERROR, "sensor_error"));
        hz.diag_single_point_fault().push_back(makeDiag(diagnostic_msgs::msg::DiagnosticStatus_Constants::STALE, "sensor_stale"));
        hz.diag_single_point_fault().push_back(makeDiag(99, "sensor_unknown"));

        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hz_stamped, false);

        // Test other levels
        hz.level(autoware_system_msgs::msg::HazardStatus_Constants::SAFE_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hz_stamped, false);

        hz.level(autoware_system_msgs::msg::HazardStatus_Constants::LATENT_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hz_stamped, false);

        hz.level(autoware_system_msgs::msg::HazardStatus_Constants::SINGLE_POINT_FAULT);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hz_stamped, false);

        hz.level(99);
        publishSample<autoware_system_msgs::msg::HazardStatusStamped, autoware_system_msgs::msg::HazardStatusStampedPubSubType>("rt/system/emergency/hazard_status", hz_stamped, false);
    }

    // Emergency
    {
        tier4_external_api_msgs::msg::Emergency emergency;
        emergency.emergency(true);
        publishSample<tier4_external_api_msgs::msg::Emergency, tier4_external_api_msgs::msg::EmergencyPubSubType>("rt/api/autoware/get/emergency", emergency, false);

        emergency.emergency(false);
        publishSample<tier4_external_api_msgs::msg::Emergency, tier4_external_api_msgs::msg::EmergencyPubSubType>("rt/api/autoware/get/emergency", emergency, false);
    }

    // OperationModeAvailability
    {
        tier4_system_msgs::msg::OperationModeAvailability op_avail;
        op_avail.autonomous(true);
        op_avail.stop(true);
        op_avail.is_local(false);
        op_avail.remote(false);
        op_avail.emergency_stop(false);
        publishSample<tier4_system_msgs::msg::OperationModeAvailability, tier4_system_msgs::msg::OperationModeAvailabilityPubSubType>("rt/system/operation_mode/availability", op_avail, false);
    }

    // OperationModeState (STOP, AUTONOMOUS, LOCAL, REMOTE, UNKNOWN)
    {
        autoware_adapi_v1_msgs::msg::OperationModeState op_state;
        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::STOP);
        op_state.is_autoware_control_enabled(true);
        op_state.is_in_transition(false);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::AUTONOMOUS);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::LOCAL_);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(autoware_adapi_v1_msgs::msg::OperationModeState_Constants::REMOTE);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);

        op_state.mode(99);
        publishSample<autoware_adapi_v1_msgs::msg::OperationModeState, autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType>("rt/api/operation_mode/state", op_state, false);
    }

    // VehicleStatus (Gears, TurnIndicators, HazardLights)
    {
        autoware_adapi_v1_msgs::msg::VehicleStatus vs;
        vs.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::PARK);
        vs.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::LEFT);
        vs.hazard_lights().status(autoware_adapi_v1_msgs::msg::HazardLights_Constants::ENABLE);
        vs.steering_tire_angle(0.15f);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", vs, false);

        vs.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::REVERSE);
        vs.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::RIGHT);
        vs.hazard_lights().status(autoware_adapi_v1_msgs::msg::HazardLights_Constants::DISABLE);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", vs, false);

        vs.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::LOW);
        vs.turn_indicators().status(autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::UNKNOWN);
        vs.hazard_lights().status(autoware_adapi_v1_msgs::msg::HazardLights_Constants::UNKNOWN);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", vs, false);

        vs.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::NEUTRAL);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", vs, false);

        vs.gear_().status(autoware_adapi_v1_msgs::msg::Gear_Constants::DRIVE);
        publishSample<autoware_adapi_v1_msgs::msg::VehicleStatus, autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType>("rt/api/vehicle/status", vs, false);
    }
}

TEST_F(DDSProviderLifecycleTest, SetValueEdgeCases) {
    provider->start();

    // Set value for supported paths
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "DRIVE");
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "REVERSE");
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "PARK");
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "NEUTRAL");
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "LOW");
    provider->setValue("Vehicle.Powertrain.Transmission.SelectedGear", "UNKNOWN_GEAR");

    provider->setValue("Vehicle.Cabin.Door.Row1.Left.IsOpen", "true");
    provider->setValue("Vehicle.Cabin.Door.Row1.Right.IsOpen", "false");

    provider->setValue("Vehicle.Speed", "100.5");
    provider->setValue("Vehicle.Speed", 100.5);

    // Unsupported path
    provider->setValue("Vehicle.Unsupported.Path", "value");

    // Subscribe and Unsubscribe
    provider->subscribe("Vehicle.Speed");
    provider->unsubscribe("Vehicle.Speed");

    // Null data channel push
    provider->setDataChannel(nullptr);
    provider->pushToDataChannel("Vehicle.Speed", 50.0);

    provider->stop();
}

TEST_F(DDSProviderHandlersTest, Trajectory_AllFields) {
    autoware_planning_msgs::msg::Trajectory traj;
    autoware_planning_msgs::msg::TrajectoryPoint pt;
    pt.pose().position().x(10.0);
    pt.pose().position().y(20.0);
    pt.pose().position().z(0.0);
    pt.pose().orientation().w(1.0);
    pt.longitudinal_velocity_mps(12.5f);
    pt.lateral_velocity_mps(0.0f);
    pt.acceleration_mps2(1.2f);
    pt.heading_rate_rps(0.05f);
    pt.front_wheel_angle_rad(0.02f);
    pt.rear_wheel_angle_rad(0.01f);
    builtin_interfaces::msg::Duration dur;
    dur.sec(1);
    dur.nanosec(500000000);
    pt.time_from_start(dur);

    traj.points().push_back(pt);
    publishSample<autoware_planning_msgs::msg::Trajectory, autoware_planning_msgs::msg::TrajectoryPubSubType>("rt/planning/scenario_planning/trajectory", traj, false);
}


