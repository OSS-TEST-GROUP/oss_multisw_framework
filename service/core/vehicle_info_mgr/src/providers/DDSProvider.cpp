#include "DDSProvider.h"
#include "Utils.h" // For any_to_string

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/builtin/topic/ParticipantBuiltinTopicData.hpp>
#include <fastdds/dds/builtin/topic/PublicationBuiltinTopicData.hpp>
#include <fastdds/dds/builtin/topic/SubscriptionBuiltinTopicData.hpp>
#include <fastdds/rtps/participant/ParticipantDiscoveryInfo.hpp>
#include <fastdds/rtps/reader/ReaderDiscoveryStatus.hpp>
#include <fastdds/rtps/writer/WriterDiscoveryStatus.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

// Include the generated DDS type support headers
#include "VehicleDataPubSubTypes.hpp"
#include "GnssInsOrientationStampedPubSubTypes.hpp" // For Autoware topic
#include "GnssInsOrientationStamped.hpp"
#include "autoware_planning_msgs/msg/TrajectoryPubSubTypes.hpp" // For Autoware trajectory
#include "autoware_planning_msgs/msg/Trajectory.hpp"
#include "autoware_planning_msgs/msg/PathPubSubTypes.hpp" // For Autoware Path
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
#include "VssTree.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <iostream>
#include <cmath> // For M_PI
#include "Logger.h"

using namespace eprosima::fastdds::dds;

namespace {
    inline long long getTimestampMs(const std_msgs::msg::Header& header) {
        return static_cast<long long>(header.stamp().sec()) * 1000 + 
               static_cast<long long>(header.stamp().nanosec()) / 1000000;
    }

    inline long long getTimestampMs(const builtin_interfaces::msg::Time& stamp) {
    return static_cast<long long>(stamp.sec()) * 1000 + 
            static_cast<long long>(stamp.nanosec()) / 1000000;
    }

    rapidjson::Value serializePoint(const geometry_msgs::msg::Point& point, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("x", point.x(), allocator);
        val.AddMember("y", point.y(), allocator);
        val.AddMember("z", point.z(), allocator);
        return val;
    }

    rapidjson::Value serializeQuaternion(const geometry_msgs::msg::Quaternion& orientation, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("x", orientation.x(), allocator);
        val.AddMember("y", orientation.y(), allocator);
        val.AddMember("z", orientation.z(), allocator);
        val.AddMember("w", orientation.w(), allocator);
        return val;
    }

    rapidjson::Value serializePose(const geometry_msgs::msg::Pose& pose, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("position", serializePoint(pose.position(), allocator), allocator);
        val.AddMember("orientation", serializeQuaternion(pose.orientation(), allocator), allocator);
        return val;
    }

    rapidjson::Value serializeVector3(const geometry_msgs::msg::Vector3& vec, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("x", vec.x(), allocator);
        val.AddMember("y", vec.y(), allocator);
        val.AddMember("z", vec.z(), allocator);
        return val;
    }

    rapidjson::Value serializeTwist(const geometry_msgs::msg::Twist& twist, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("linear", serializeVector3(twist.linear(), allocator), allocator);
        val.AddMember("angular", serializeVector3(twist.angular(), allocator), allocator);
        return val;
    }

    rapidjson::Value serializeAccel(const geometry_msgs::msg::Accel& accel, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("linear", serializeVector3(accel.linear(), allocator), allocator);
        val.AddMember("angular", serializeVector3(accel.angular(), allocator), allocator);
        return val;
    }

    rapidjson::Value serializeGeoPoint(const geographic_msgs::msg::GeoPoint& geo_point, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("latitude", geo_point.latitude(), allocator);
        val.AddMember("longitude", geo_point.longitude(), allocator);
        val.AddMember("altitude", geo_point.altitude(), allocator);
        return val;
    }

    rapidjson::Value serializePoseWithCovariance(const geometry_msgs::msg::PoseWithCovariance& pose_cov, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("pose", serializePose(pose_cov.pose_(), allocator), allocator);
        rapidjson::Value cov_array(rapidjson::kArrayType);
        for (int i = 0; i < 36; ++i) {
            cov_array.PushBack(pose_cov.covariance()[i], allocator);
        }
        val.AddMember("covariance", cov_array, allocator);
        return val;
    }

    rapidjson::Value serializeTwistWithCovariance(const geometry_msgs::msg::TwistWithCovariance& twist_cov, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("twist", serializeTwist(twist_cov.twist_(), allocator), allocator);
        rapidjson::Value cov_array(rapidjson::kArrayType);
        for (int i = 0; i < 36; ++i) {
            cov_array.PushBack(twist_cov.covariance()[i], allocator);
        }
        val.AddMember("covariance", cov_array, allocator);
        return val;
    }

    rapidjson::Value serializeAccelWithCovariance(const geometry_msgs::msg::AccelWithCovariance& accel_cov, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("accel", serializeAccel(accel_cov.accel_(), allocator), allocator);
        rapidjson::Value cov_array(rapidjson::kArrayType);
        for (int i = 0; i < 36; ++i) {
            cov_array.PushBack(accel_cov.covariance()[i], allocator);
        }
        val.AddMember("covariance", cov_array, allocator);
        return val;
    }

    // rapidjson::Value를 shared_ptr<Document>로 변환하는 유틸리티
    // Value의 복사본을 생성하므로 원본 Value는 그대로 유지됨
    std::shared_ptr<rapidjson::Document> toDocumentPtr(const rapidjson::Value& value, rapidjson::Document::AllocatorType& src_allocator) {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->CopyFrom(value, doc->GetAllocator());
        return doc;
    }

    rapidjson::Value serializeDuration(const builtin_interfaces::msg::Duration& duration, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("sec", duration.sec(), allocator);
        val.AddMember("nanosec", duration.nanosec(), allocator);
        return val;
    }

    rapidjson::Value serializeDynamicObjectPath(const autoware_adapi_v1_msgs::msg::DynamicObjectPath& path_msg, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        
        rapidjson::Value path_array(rapidjson::kArrayType);
        for (const auto& pose : path_msg.path()) {
            path_array.PushBack(serializePose(pose, allocator), allocator);
        }
        val.AddMember("path", path_array, allocator);
        val.AddMember("time_step", serializeDuration(path_msg.time_step(), allocator), allocator);
        val.AddMember("confidence", path_msg.confidence(), allocator);
        return val;
    }

    rapidjson::Value serializeKinematics(const autoware_adapi_v1_msgs::msg::DynamicObjectKinematics& kinematics, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("pose", serializePose(kinematics.pose(), allocator), allocator);
        val.AddMember("twist", serializeTwist(kinematics.twist(), allocator), allocator);
        val.AddMember("accel", serializeAccel(kinematics.accel(), allocator), allocator);

        rapidjson::Value paths_array(rapidjson::kArrayType);
        for (const auto& path : kinematics.predicted_paths()) {
            paths_array.PushBack(serializeDynamicObjectPath(path, allocator), allocator);
        }
        val.AddMember("predicted_paths", paths_array, allocator);
        return val;
    }


    rapidjson::Value serializeDetectedObjectKinematics(const autoware_perception_msgs::msg::DetectedObjectKinematics& kinematics, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("pose_with_covariance", serializePoseWithCovariance(kinematics.pose_with_covariance(), allocator), allocator);
        val.AddMember("has_position_covariance", rapidjson::Value(kinematics.has_position_covariance() ? "true" : "false", allocator), allocator);

        if (kinematics.orientation_availability() == autoware_perception_msgs::msg::DetectedObjectKinematics_Constants::SIGN_UNKNOWN) {
            val.AddMember("orientation_availability", rapidjson::Value("SIGN_UNKNOWN", allocator), allocator);
        } else if (kinematics.orientation_availability() == autoware_perception_msgs::msg::DetectedObjectKinematics_Constants::AVAILABLE) {
            val.AddMember("orientation_availability", rapidjson::Value("AVAILABLE", allocator), allocator);
        } else {
            val.AddMember("orientation_availability", rapidjson::Value("UNAVAILABLE", allocator), allocator);
        }

        val.AddMember("twist_with_covariance", serializeTwistWithCovariance(kinematics.twist_with_covariance(), allocator), allocator);
        val.AddMember("has_twist", rapidjson::Value(kinematics.has_twist() ? "true" : "false", allocator), allocator);
        val.AddMember("has_twist_covariance", rapidjson::Value(kinematics.has_twist_covariance() ? "true" : "false", allocator), allocator);

        return val;
    }


    rapidjson::Value serializePoint32(const geometry_msgs::msg::Point32& pt, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        val.AddMember("x", pt.x(), allocator);
        val.AddMember("y", pt.y(), allocator);
        val.AddMember("z", pt.z(), allocator);
        return val;
    }

    rapidjson::Value serializePolygon(const geometry_msgs::msg::Polygon& polygon, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        rapidjson::Value pts_array(rapidjson::kArrayType);
        for (const auto& pt : polygon.points()) {
            pts_array.PushBack(serializePoint32(pt, allocator), allocator);
        }
        val.AddMember("points", pts_array, allocator);
        return val;
    }

    rapidjson::Value serializeSolidPrimitive(const shape_msgs::msg::SolidPrimitive& shape, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        
        std::string type_str;
        switch (shape.type()) {
            case shape_msgs::msg::SolidPrimitive_Constants::BOX:
                type_str = "box";
                break;
            case shape_msgs::msg::SolidPrimitive_Constants::SPHERE:
                type_str = "sphere";
                break;
            case shape_msgs::msg::SolidPrimitive_Constants::CYLINDER:
                type_str = "cylinder";
                break;
            case shape_msgs::msg::SolidPrimitive_Constants::CONE:
                type_str = "cone";
                break;
            case shape_msgs::msg::SolidPrimitive_Constants::PRISM:
                type_str = "prism";
                break;
            default:
                type_str = "unknown";
                break;
        }
        
        val.AddMember("type", rapidjson::Value(type_str.c_str(), allocator), allocator);
        
        rapidjson::Value dims_array(rapidjson::kArrayType);
        for (const auto& dim : shape.dimensions()) {
            dims_array.PushBack(dim, allocator);
        }
        val.AddMember("dimensions", dims_array, allocator);
        val.AddMember("polygon", serializePolygon(shape.polygon(), allocator), allocator);
        return val;
    }

    rapidjson::Value serializeShape(const autoware_perception_msgs::msg::Shape& shape, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value val(rapidjson::kObjectType);
        
        std::string type_str;
        switch (shape.type()) {
            case autoware_perception_msgs::msg::Shape_Constants::BOUNDING_BOX:
                type_str = "BOUNDING_BOX";
                break;
            case autoware_perception_msgs::msg::Shape_Constants::CYLINDER:
                type_str = "CYLINDER";
                break;
            case autoware_perception_msgs::msg::Shape_Constants::POLYGON:
                type_str = "POLYGON";
                break;
            default:
                type_str = "unknown";
                break;
        }
        val.AddMember("type", rapidjson::Value(type_str.c_str(), allocator), allocator);
        val.AddMember("footprint", serializePolygon(shape.footprint(), allocator), allocator);
        val.AddMember("dimensions", serializeVector3(shape.dimensions(), allocator), allocator);
        return val;
    }

    // DiagnosticStatus 벡터를 RapidJSON Document(Array)로 직렬화하는 공통 헬퍼
    std::shared_ptr<rapidjson::Document> serializeDiagnostics(
            const std::vector<diagnostic_msgs::msg::DiagnosticStatus>& diag_list)
    {
        auto doc = std::make_shared<rapidjson::Document>();
        doc->SetArray();
        auto& allocator = doc->GetAllocator();

        for (const auto& diag : diag_list) {
            rapidjson::Value diag_val(rapidjson::kObjectType);

            auto level = diag.level();
            std::string level_str;

            if(level == diagnostic_msgs::msg::DiagnosticStatus_Constants::OK){
                level_str = "OK";
            } else if (level == diagnostic_msgs::msg::DiagnosticStatus_Constants::WARN){
                level_str = "WARN";
            } else if (level == diagnostic_msgs::msg::DiagnosticStatus_Constants::ERROR){
                level_str = "ERROR";
            } else if (level == diagnostic_msgs::msg::DiagnosticStatus_Constants::STALE){
                level_str = "STALE";
            } else {
                level_str = "Unknown";
            }

            diag_val.AddMember("level", rapidjson::Value(level_str.c_str(), allocator), allocator);
            diag_val.AddMember("name", rapidjson::Value(diag.name().c_str(), allocator), allocator);
            diag_val.AddMember("message", rapidjson::Value(diag.message().c_str(), allocator), allocator);
            diag_val.AddMember("hardware_id", rapidjson::Value(diag.hardware_id().c_str(), allocator), allocator);

            rapidjson::Value values_arr(rapidjson::kArrayType);
            for (const auto& kv : diag.values()) {
                rapidjson::Value kv_val(rapidjson::kObjectType);
                kv_val.AddMember("key",
                    rapidjson::Value(kv.key().c_str(), allocator), allocator);
                kv_val.AddMember("value",
                    rapidjson::Value(kv.value().c_str(), allocator), allocator);
                values_arr.PushBack(kv_val, allocator);
            }
            diag_val.AddMember("values", values_arr, allocator);
            doc->PushBack(diag_val, allocator);
        }
        return doc;
    }

    rapidjson::Value serializeCooperationStatus(
            const autoware_adapi_v1_msgs::msg::CooperationStatus& cooperation_status,
            rapidjson::Document::AllocatorType& allocator)
    {
        const std::array<uint8_t, 16>& uuid = cooperation_status.uuid().uuid();
        auto autonomous = cooperation_status.autonomous().decision();
        auto cooperator = cooperation_status.cooperator().decision();
        bool cancellable = cooperation_status.cancellable();
        
        rapidjson::Value cooperation_status_val(rapidjson::kObjectType);

        rapidjson::Value uuid_array(rapidjson::kArrayType);
        for(const auto& u : uuid){
            uuid_array.PushBack(u, allocator);
        }
        cooperation_status_val.AddMember("uuid", uuid_array, allocator);

        if(autonomous == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::DEACTIVATE){
            cooperation_status_val.AddMember("autonomous", rapidjson::Value("DEACTIVATE", allocator), allocator);
        }else if(autonomous == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::ACTIVATE){
            cooperation_status_val.AddMember("autonomous", rapidjson::Value("ACTIVATE", allocator), allocator);
        }else if(autonomous == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::AUTONOMOUS){
            cooperation_status_val.AddMember("autonomous", rapidjson::Value("AUTONOMOUS", allocator), allocator);
        }else if(autonomous == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNDECIDED){
            cooperation_status_val.AddMember("autonomous", rapidjson::Value("UNDECIDED", allocator), allocator);
        }else {// autonomous == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNKNOWN
            cooperation_status_val.AddMember("autonomous", rapidjson::Value("UNKNOWN", allocator), allocator);
        }

        if(cooperator == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::DEACTIVATE){
            cooperation_status_val.AddMember("cooperator", rapidjson::Value("DEACTIVATE", allocator), allocator);
        }else if(cooperator == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::ACTIVATE){
            cooperation_status_val.AddMember("cooperator", rapidjson::Value("ACTIVATE", allocator), allocator);
        }else if(cooperator == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::AUTONOMOUS){
            cooperation_status_val.AddMember("cooperator", rapidjson::Value("AUTONOMOUS", allocator), allocator);
        }else if(cooperator == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNDECIDED){
            cooperation_status_val.AddMember("cooperator", rapidjson::Value("UNDECIDED", allocator), allocator);
        }else {// cooperator == autoware_adapi_v1_msgs::msg::CooperationDecision_Constants::UNKNOWN
            cooperation_status_val.AddMember("cooperator", rapidjson::Value("UNKNOWN", allocator), allocator);
        }

        cooperation_status_val.AddMember("cancellable", rapidjson::Value(cancellable ? "true" : "false", allocator), allocator);

        return cooperation_status_val;
    }

    rapidjson::Value serializeCooperationStatusArray(
            const std::vector<autoware_adapi_v1_msgs::msg::CooperationStatus>& cooperation_list,
            rapidjson::Document::AllocatorType& allocator)
    {
        rapidjson::Value cooperation_status_array(rapidjson::kArrayType);

        for (const auto& coop : cooperation_list)
        {
            cooperation_status_array.PushBack(
                serializeCooperationStatus(coop, allocator),
                allocator
            );
        }

        return cooperation_status_array;
    }
}

// Listener for DDS Discovery
class DDSProvider::DiscoveryListener : public DomainParticipantListener
{
public:
    void on_participant_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::ParticipantDiscoveryStatus reason,
            const ParticipantBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch (reason)
        {
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DISCOVERED_PARTICIPANT:
                LOG_INFO << "[DDS Discovery] Discovered Participant: " << info.participant_name.to_string();
                break;
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::REMOVED_PARTICIPANT:
            case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DROPPED_PARTICIPANT:
                LOG_INFO << "[DDS Discovery] Removed or Dropped Participant: " << info.participant_name.to_string();
                break;
            default:
                break;
        }
    }

    void on_data_writer_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
            const PublicationBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch(reason)
        {
            case eprosima::fastdds::rtps::WriterDiscoveryStatus::DISCOVERED_WRITER:
                LOG_INFO << "[DDS Discovery] Discovered Publisher for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
            case eprosima::fastdds::rtps::WriterDiscoveryStatus::REMOVED_WRITER:
                LOG_INFO << "[DDS Discovery] Removed Publisher for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
        }
    }

    void on_data_reader_discovery(
            DomainParticipant* participant,
            eprosima::fastdds::rtps::ReaderDiscoveryStatus reason,
            const SubscriptionBuiltinTopicData& info,
            bool& should_be_ignored) override
    {
        (void)participant;
        (void)should_be_ignored;
        switch(reason)
        {
            case eprosima::fastdds::rtps::ReaderDiscoveryStatus::DISCOVERED_READER:
                LOG_INFO << "[DDS Discovery] Discovered Subscriber for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
            case eprosima::fastdds::rtps::ReaderDiscoveryStatus::REMOVED_READER:
                LOG_INFO << "[DDS Discovery] Removed Subscriber for topic '" << info.topic_name << "' with type '" << info.type_name << "'";
                break;
        }
    }
};

// Implement Common DDSDataReaderListener
void DDSProvider::DDSDataReaderListener::on_subscription_matched(
        DataReader* reader,
        const SubscriptionMatchedStatus& info)
{
    const auto participant = reader->get_subscriber()->get_participant();
    const std::string participant_name = participant->get_qos().name().to_string();

    if (info.current_count_change > 0)
    {
        LOG_INFO << "******************** [DDS Matched] DataReader matched on participant [" << participant_name
                 << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
    }
    else
    {
        LOG_INFO << "******************** [DDS Unmatched] DataReader unmatched on participant [" << participant_name
                 << "] for topic: [" << reader->get_topicdescription()->get_name() << "]";
    }
}

void DDSProvider::DDSDataReaderListener::on_data_available(DataReader* reader)
{
    if (callback_)
    {
        callback_(reader);
    }
}

// Listener for VehicleDataWriter
class DDSProvider::VehicleDataWriterListener : public DataWriterListener
{
public:
    void on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& info) override
    {
        const auto participant = writer->get_publisher()->get_participant();
        const std::string participant_name = participant->get_qos().name().to_string();

        if (info.current_count_change == 1)
        {
            LOG_INFO << "******************** [DDS Matched] DataWriter matched on participant [" << participant_name
                     << "] for topic: [" << writer->get_topic()->get_name() << "]";
        }
        else if (info.current_count_change == -1)
        {
            LOG_INFO << "******************** [DDS Unmatched] DataWriter unmatched on participant [" << participant_name
                     << "] for topic: [" << writer->get_topic()->get_name() << "]";
        }
    }
};


DDSProvider::DDSProvider()
    : participant_(nullptr)
    , publisher_(nullptr)
    , subscriber_(nullptr)
    // , vehicle_data_topic_(nullptr)
    // , set_vehicle_data_topic_(nullptr)
    // , vehicle_data_writer_(nullptr)
    // , vehicle_data_reader_(nullptr)
    // , vehicle_data_type_(new VehicleDataTopicPubSubType())
    , active_(false)
    , discovery_listener_(std::make_unique<DiscoveryListener>())
{
    // Determine Domain ID from environment variable, default to 0
    int domain_id = 0;
    const char* domain_id_str = std::getenv("VISS_DDS_DOMAIN_ID");
    if (domain_id_str) {
        try {
            domain_id = std::stoi(domain_id_str);
        } catch (const std::invalid_argument& e) {
            LOG_WARN << "Invalid VISS_DDS_DOMAIN_ID: " << domain_id_str << ". Using default 0.";
            domain_id = 0;
        } catch (const std::out_of_range& e) {
            LOG_WARN << "VISS_DDS_DOMAIN_ID out of range: " << domain_id_str << ". Using default 0.";
            domain_id = 0;
        }
    }

    // Create Participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("VISS_Participant");
    participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos, discovery_listener_.get(), StatusMask::none());
    if (participant_ == nullptr) {
        LOG_ERROR << "Failed to create DDS Participant on domain " << domain_id;
        return;
    }

    // Create Publisher and Subscriber
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, StatusMask::none());
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, StatusMask::none());

    // Register Subscriptions (Topic, Type, QoS, and Callback mapping)
    registerSubscription("rt/autoware_orientation", 
        TypeSupport(new autoware_sensing_msgs::msg::GnssInsOrientationStampedPubSubType()), 
        false, 
        [this](DataReader* reader) { handleGnssOrientation(reader); });

    registerSubscription("rt/planning/scenario_planning/trajectory", 
        TypeSupport(new autoware_planning_msgs::msg::TrajectoryPubSubType()), 
        false, 
        [this](DataReader* reader) { handleTrajectory(reader); });

    registerSubscription("rt/api/vehicle/kinematics", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::VehicleKinematicsPubSubType()), 
        true, 
        [this](DataReader* reader) { handleVehicleKinematics(reader); });

    registerSubscription("rt/api/vehicle/status", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::VehicleStatusPubSubType()), 
        false, 
        [this](DataReader* reader) { handleVehicleStatus(reader); });

    registerSubscription("rt/api/routing/state", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::RouteStatePubSubType()), 
        false, 
        [this](DataReader* reader) { handleRouteState(reader); });

    registerSubscription("rt/api/routing/route", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::RoutePubSubType()), 
        false, 
        [this](DataReader* reader) { handleRoute(reader); });

    registerSubscription("rt/api/fail_safe/mrm_state", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::MrmStatePubSubType()), 
        false, 
        [this](DataReader* reader) { handleMrmState(reader); });

    registerSubscription("rt/api/autoware/get/emergency", 
        TypeSupport(new tier4_external_api_msgs::msg::EmergencyPubSubType()), 
        false, 
        [this](DataReader* reader) { handleEmergency(reader); });

    registerSubscription("rt/system/emergency/hazard_status", 
        TypeSupport(new autoware_system_msgs::msg::HazardStatusStampedPubSubType()), 
        false, 
        [this](DataReader* reader) { handleHazardStatus(reader); });

    registerSubscription("rt/system/operation_mode/availability", 
        TypeSupport(new tier4_system_msgs::msg::OperationModeAvailabilityPubSubType()), 
        false, 
        [this](DataReader* reader) { handleOperationModeAvailability(reader); });

    registerSubscription("rt/api/operation_mode/state", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::OperationModeStatePubSubType()), 
        false, 
        [this](DataReader* reader) { handleOperationModeState(reader); });

    registerSubscription("rt/api/perception/objects", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::DynamicObjectArrayPubSubType()), 
        false, 
        [this](DataReader* reader) { handleDynamicObjectArray(reader); });

    registerSubscription("rt/api/planning/velocity_factors", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::VelocityFactorArrayPubSubType()), 
        false, 
        [this](DataReader* reader) { handleVelocityFactorArray(reader); });

    registerSubscription("rt/api/planning/steering_factors", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::SteeringFactorArrayPubSubType()), 
        false, 
        [this](DataReader* reader) { handleSteeringFactorArray(reader); });

    registerSubscription("rt/perception/object_recognition/detection/objects", 
        TypeSupport(new autoware_perception_msgs::msg::DetectedObjectsPubSubType()), 
        false, 
        [this](DataReader* reader) { handleDetectedObjects(reader); });

    registerSubscription("rt/planning/scenario_planning/lane_driving/motion_planning/path_smoother/path", 
        TypeSupport(new autoware_planning_msgs::msg::PathPubSubType()), 
        false, 
        [this](DataReader* reader) { handlePath(reader); });

    registerSubscription("rt/perception/traffic_light_recognition/traffic_signals", 
        TypeSupport(new autoware_perception_msgs::msg::TrafficLightGroupArrayPubSubType()), 
        false, 
        [this](DataReader* reader) { handleTrafficLightGroupArray(reader); });

    registerSubscription("rt/api/system/diagnostics/status", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::DiagGraphStatusPubSubType()), 
        true, 
        [this](DataReader* reader) { handleDiagGraphStatus(reader); });

    registerSubscription("rt/api/system/diagnostics/struct", 
        TypeSupport(new autoware_adapi_v1_msgs::msg::DiagGraphStructPubSubType()), 
        true, 
        [this](DataReader* reader) { handleDiagGraphStruct(reader); });


}

void DDSProvider::registerSubscription(
    const std::string& topic_name,
    eprosima::fastdds::dds::TypeSupport type_support,
    bool best_effort,
    std::function<void(eprosima::fastdds::dds::DataReader*)> callback)
{
    if (participant_ == nullptr) return;

    auto sub = std::make_shared<Subscription>();
    sub->topic_name = topic_name;
    sub->type_support = type_support;
    sub->best_effort = best_effort;
    sub->callback = callback;

    // Type 등록
    sub->type_support.register_type(participant_);

    // Topic 생성
    sub->topic = participant_->create_topic(topic_name, sub->type_support.get_type_name(), TOPIC_QOS_DEFAULT);
    if (sub->topic == nullptr) {
        LOG_ERROR << "Failed to create topic: " << topic_name;
        return;
    }

    subscriptions_.push_back(sub);
}

void DDSProvider::pushToDataChannel(const std::string& path, const std::any& value, long long timestamp)
{
    if (!data_channel_) return;

    VehicleData data;
    data.path = path;
    data.value = value;
    if (timestamp == 0) {
        data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    } else {
        data.timestamp = timestamp;
    }

    data_channel_->push(data);
}

DDSProvider::~DDSProvider()
{
    stop();
    if (participant_ != nullptr)
    {
        participant_->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void DDSProvider::start()
{
    if (participant_ == nullptr) return;

    for (auto& sub : subscriptions_)
    {
        sub->listener = std::make_unique<DDSDataReaderListener>(sub->callback);

        DataReaderQos qos = DATAREADER_QOS_DEFAULT;
        if (sub->best_effort)
        {
            qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        }
        else
        {
            qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        }

        sub->reader = subscriber_->create_datareader(sub->topic, qos, sub->listener.get());
        if (sub->reader == nullptr)
        {
            LOG_ERROR << "Failed to create DataReader for topic: " << sub->topic_name;
        }
    }

    active_ = true;
    LOG_INFO << "DDSProvider started.";
}

void DDSProvider::stop()
{
    if (!active_.exchange(false)) {
        return;
    }
    if (subscriber_ != nullptr)
    {
        for (auto& sub : subscriptions_)
        {
            if (sub->reader != nullptr)
            {
                subscriber_->delete_datareader(sub->reader);
                sub->reader = nullptr;
            }
            sub->listener.reset();
        }
    }
    // if (publisher_ != nullptr && vehicle_data_writer_ != nullptr)
    // {
    //     publisher_->delete_datawriter(vehicle_data_writer_);
    // }
    LOG_INFO << "DDSProvider stopped.";
}

void DDSProvider::handleDiagGraphStruct(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::DiagGraphStruct sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "DiagGraphStruct";
            rapidjson::Document diag_graph_struct_doc;
            diag_graph_struct_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = diag_graph_struct_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.stamp());
            const auto& id = sample.id();
            const auto& nodes = sample.nodes();
            const auto& links = sample.links();

            
            diag_graph_struct_doc.AddMember("id", rapidjson::Value(id.c_str(), allocator), allocator);

            rapidjson::Document nodes_array(rapidjson::kArrayType);
            for (const auto& node : nodes)
            {
                rapidjson::Document node_doc;
                node_doc.SetObject();
                auto path = node.path();
                node_doc.AddMember("path", rapidjson::Value(path.c_str(), allocator), allocator);
                nodes_array.PushBack(node_doc, allocator);
            }
            diag_graph_struct_doc.AddMember("nodes", nodes_array, allocator);


            rapidjson::Document links_array(rapidjson::kArrayType);
            for (const auto& link : links)
            {
                rapidjson::Document link_doc;
                link_doc.SetObject();
                link_doc.AddMember("parent", link.parent(), allocator);
                link_doc.AddMember("child", link.child(), allocator);
                links_array.PushBack(link_doc, allocator);
            }
            diag_graph_struct_doc.AddMember("links", links_array, allocator);



            auto diag_graph_struct_ptr = std::make_shared<rapidjson::Document>(std::move(diag_graph_struct_doc));
            pushToDataChannel("Vehicle.ADS.Diagnostics.GraphStruct", diag_graph_struct_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handleDiagGraphStatus(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::DiagGraphStatus sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "DiagGraphStatus";
            rapidjson::Document diag_graph_status_doc;
            diag_graph_status_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = diag_graph_status_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.stamp());
            const auto& id = sample.id();
            const auto& nodes = sample.nodes();
            size_t node_size = nodes.size();

            pushToDataChannel("Vehicle.ADS.Diagnostics.Count", node_size, ms_since_epoch);

            diag_graph_status_doc.AddMember("id", rapidjson::Value(id.c_str(), allocator), allocator);

            rapidjson::Document nodes_array(rapidjson::kArrayType);
            for (const auto& node : nodes)
            {
                rapidjson::Document node_doc;
                node_doc.SetObject();


                auto level = node.level();
                std::string level_str;
                if(level == 0) { level_str = "OK"; }
                else if(level == 1) { level_str = "WARN"; }
                else if(level == 3) { level_str = "STALE"; }
                else { level_str = "ERROR"; }
                
                node_doc.AddMember("level", rapidjson::Value(level_str.c_str(), allocator), allocator);
                nodes_array.PushBack(node_doc, allocator);
            }
            diag_graph_status_doc.AddMember("nodes", nodes_array, allocator);


            auto diag_graph_status_ptr = std::make_shared<rapidjson::Document>(std::move(diag_graph_status_doc));
            pushToDataChannel("Vehicle.ADS.Diagnostics.GraphStatus", diag_graph_status_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handleTrafficLightGroupArray(DataReader* reader)
{
    SampleInfo info;
    autoware_perception_msgs::msg::TrafficLightGroupArray sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "TrafficLightGroupArray";
            rapidjson::Document traffic_light_group_array_doc;
            traffic_light_group_array_doc.SetArray();
            rapidjson::Document::AllocatorType& allocator = traffic_light_group_array_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.stamp());
            const auto& traffic_light_groups = sample.traffic_light_groups();
            size_t count = traffic_light_groups.size();
            

            pushToDataChannel("Vehicle.ADS.Perception.TrafficLight.LightCount", count, ms_since_epoch);
            
            for (const auto& traffic_light_group : traffic_light_groups)
            {
                rapidjson::Document traffic_light_group_doc;
                traffic_light_group_doc.SetObject();
                traffic_light_group_doc.AddMember("traffic_light_group_id", traffic_light_group.traffic_light_group_id(), allocator);


                const auto& traffic_light_elements = traffic_light_group.elements();
                rapidjson::Document traffic_light_elements_array(rapidjson::kArrayType);
                for (const auto& traffic_light_element : traffic_light_elements)
                {
                    rapidjson::Document traffic_light_element_doc;
                    traffic_light_element_doc.SetObject();

                    std::string color_str = "UNKNOWN";
                    switch (traffic_light_element.color())
                    {
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::RED:
                        color_str = "RED";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::AMBER:
                        color_str = "AMBER";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::GREEN:
                        color_str = "GREEN";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::WHITE:
                        color_str = "WHITE";
                        break;
                    }

                    std::string shape_str = "UNKNOWN";
                    switch (traffic_light_element.shape())
                    {
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::CIRCLE:
                        shape_str = "CIRCLE";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::LEFT_ARROW:
                        shape_str = "LEFT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::RIGHT_ARROW:
                        shape_str = "RIGHT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_ARROW:
                        shape_str = "UP_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_LEFT_ARROW:
                        shape_str = "UP_LEFT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::UP_RIGHT_ARROW:
                        shape_str = "UP_RIGHT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_ARROW:
                        shape_str = "DOWN_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_LEFT_ARROW:
                        shape_str = "DOWN_LEFT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::DOWN_RIGHT_ARROW:
                        shape_str = "DOWN_RIGHT_ARROW";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::CROSS:
                        shape_str = "CROSS";
                        break;
                    }

                    std::string status_str = "UNKNOWN";
                    switch (traffic_light_element.status())
                    {
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_OFF:
                        status_str = "SOLID_OFF";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::SOLID_ON:
                        status_str = "SOLID_ON";
                        break;
                    case autoware_perception_msgs::msg::TrafficLightElement_Constants::FLASHING:
                        status_str = "FLASHING";
                        break;
                    }

                    traffic_light_element_doc.AddMember("color", rapidjson::Value(color_str.c_str(), allocator), allocator);
                    traffic_light_element_doc.AddMember("shape", rapidjson::Value(shape_str.c_str(), allocator), allocator);
                    traffic_light_element_doc.AddMember("status", rapidjson::Value(status_str.c_str(), allocator), allocator);
                    traffic_light_element_doc.AddMember("confidence", traffic_light_element.confidence(), allocator);
                    traffic_light_elements_array.PushBack(traffic_light_element_doc, allocator);
                }
                traffic_light_group_doc.AddMember("elements", traffic_light_elements_array, allocator);
                traffic_light_group_array_doc.PushBack(traffic_light_group_doc, allocator);
            }
            auto traffic_light_group_array_ptr = std::make_shared<rapidjson::Document>(std::move(traffic_light_group_array_doc));
            pushToDataChannel("Vehicle.ADS.Perception.TrafficLight.Lights", traffic_light_group_array_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handlePath(DataReader* reader)
{
    SampleInfo info;
    autoware_planning_msgs::msg::Path sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "Path";
            rapidjson::Document path_doc;
            path_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = path_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.header());
            const auto& points = sample.points();
            const auto& left_bound = sample.left_bound();
            const auto& right_bound = sample.right_bound();
            
            rapidjson::Document point_array(rapidjson::kArrayType);
            for (const auto& point : points)
            {
                rapidjson::Document point_doc;
                point_doc.SetObject();
                point_doc.AddMember("pose", serializePose(point.pose(), allocator), allocator);
                point_doc.AddMember("longitudinal_velocity_mps", point.longitudinal_velocity_mps(), allocator);
                point_doc.AddMember("lateral_velocity_mps", point.lateral_velocity_mps(), allocator);
                point_doc.AddMember("heading_rate_rps", point.heading_rate_rps(), allocator);
                point_doc.AddMember("is_final", rapidjson::Value(point.is_final() ? "true" : "false", allocator), allocator);
                point_array.PushBack(point_doc, allocator);
            }
            path_doc.AddMember("points", point_array, allocator);


            rapidjson::Document left_bound_array(rapidjson::kArrayType);
            for (const auto& left : left_bound)
            {
                left_bound_array.PushBack(serializePoint(left, allocator), allocator);
            }
            path_doc.AddMember("left_bound", left_bound_array, allocator);

            rapidjson::Document right_bound_array(rapidjson::kArrayType);
            for (const auto& right : right_bound)
            {
                right_bound_array.PushBack(serializePoint(right, allocator), allocator);
            }
            path_doc.AddMember("right_bound", right_bound_array, allocator);


            auto path_ptr = std::make_shared<rapidjson::Document>(std::move(path_doc));
            pushToDataChannel("Vehicle.ADS.Planning.Path", path_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handleDetectedObjects(DataReader* reader)
{
    SampleInfo info;
    autoware_perception_msgs::msg::DetectedObjects sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "DetectedObjects";
            rapidjson::Document detected_objects_doc;
            detected_objects_doc.SetArray();
            rapidjson::Document::AllocatorType& allocator = detected_objects_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.header());
            const auto& objects = sample.objects(); 
            uint32_t count = objects.size();

            pushToDataChannel("Vehicle.ADS.Perception.DetectedObject.ObjectCount", count, ms_since_epoch);

            for (const auto& detected_object : objects)
            {
                rapidjson::Value detected_object_doc(rapidjson::kObjectType);

                detected_object_doc.AddMember("existence_probability", detected_object.existence_probability(), allocator);

                rapidjson::Value classification_array(rapidjson::kArrayType);
                for(const auto& c : detected_object.classification()){
                    uint8_t label = c.label();
                    float probability = c.probability();

                    std::string label_str;
                    if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::CAR){
                        label_str = "CAR";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::TRUCK){
                        label_str = "TRUCK";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::BUS){
                        label_str = "BUS";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::TRAILER){
                        label_str = "TRAILER";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::MOTORCYCLE){
                        label_str = "MOTORCYCLE";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::BICYCLE){
                        label_str = "BICYCLE";
                    }else if(label == autoware_perception_msgs::msg::ObjectClassification_Constants::PEDESTRIAN){
                        label_str = "PEDESTRIAN";
                    }else{
                        label_str = "UNKNOWN";
                    }

                    rapidjson::Value c_value(rapidjson::kObjectType);
                    c_value.AddMember("label", rapidjson::Value(label_str.c_str(), allocator), allocator);
                    c_value.AddMember("probability", rapidjson::Value(probability), allocator);

                    classification_array.PushBack(c_value, allocator);
    
                }
                detected_object_doc.AddMember("classification", classification_array, allocator);



                autoware_perception_msgs::msg::DetectedObjectKinematics kinematics = detected_object.kinematics();
                rapidjson::Value kinematics_value = serializeDetectedObjectKinematics(kinematics, allocator);
                detected_object_doc.AddMember("kinematics", kinematics_value, allocator);

                autoware_perception_msgs::msg::Shape shape = detected_object.shape_();
                rapidjson::Value shape_value = serializeShape(shape, allocator);
                detected_object_doc.AddMember("shape", shape_value, allocator);


                detected_objects_doc.PushBack(detected_object_doc, allocator);
            }
            auto detected_objects_ptr = std::make_shared<rapidjson::Document>(std::move(detected_objects_doc));
            pushToDataChannel("Vehicle.ADS.Perception.DetectedObject.Objects", detected_objects_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handleSteeringFactorArray(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::SteeringFactorArray sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "SteeringFactorArray";
            rapidjson::Document steering_factor_array_doc;
            steering_factor_array_doc.SetArray();
            rapidjson::Document::AllocatorType& allocator = steering_factor_array_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.header());
            std::vector<autoware_adapi_v1_msgs::msg::SteeringFactor> steering_factors = sample.factors();
            uint32_t count = steering_factors.size();
            pushToDataChannel("Vehicle.ADS.Planning.SteeringFactor.Count", count, ms_since_epoch);
            LOG_DEBUG << "Vehicle.ADS.Planning.SteeringFactor.Count:" << count;


            for (const auto& factor : steering_factors)
            {
                rapidjson::Value factor_val(rapidjson::kObjectType);
                const std::array<geometry_msgs::msg::Pose, 2>& poses = factor.pose();
                const std::array<float, 2>& distance = factor.distance();

                rapidjson::Value pose_array(rapidjson::kArrayType);
                for (const auto& pose : poses) {
                    pose_array.PushBack(serializePose(pose, allocator), allocator);
                }
                factor_val.AddMember("pose", pose_array, allocator);

                rapidjson::Value distance_array(rapidjson::kArrayType);
                for (const auto& d : distance) {
                    distance_array.PushBack(d, allocator);
                }
                factor_val.AddMember("distance", distance_array, allocator);



                uint16_t direction = factor.direction();
                if(direction == autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::LEFT) {
                    factor_val.AddMember("direction", rapidjson::Value("LEFT", allocator), allocator);
                } else if(direction == autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::RIGHT) {
                    factor_val.AddMember("direction", rapidjson::Value("RIGHT", allocator), allocator);
                } else if(direction == autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::STRAIGHT) {
                    factor_val.AddMember("direction", rapidjson::Value("STRAIGHT", allocator), allocator);
                }else {// autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::UNKNOWN
                    factor_val.AddMember("direction", rapidjson::Value("UNKNOWN", allocator), allocator);
                }

                uint16_t status = factor.status();
                if(status == autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::APPROACHING) {
                    factor_val.AddMember("status", rapidjson::Value("APPROACHING", allocator), allocator);
                } else if(status == autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::TURNING) {
                    factor_val.AddMember("status", rapidjson::Value("TURNING", allocator), allocator);
                } else {// autoware_adapi_v1_msgs::msg::SteeringFactor_Constants::UNKNOWN
                    factor_val.AddMember("status", rapidjson::Value("UNKNOWN", allocator), allocator);
                }

                factor_val.AddMember("behavior", rapidjson::Value(factor.behavior().c_str(), allocator), allocator);
                factor_val.AddMember("sequence", rapidjson::Value(factor.sequence_().c_str(), allocator), allocator);
                factor_val.AddMember("detail", rapidjson::Value(factor.detail().c_str(), allocator), allocator);

                rapidjson::Value cooperation_status_array = serializeCooperationStatusArray(factor.cooperation(), allocator);
                factor_val.AddMember("cooperation", cooperation_status_array, allocator);

                steering_factor_array_doc.PushBack(factor_val, allocator);
            }

            auto doc_ptr = std::make_shared<rapidjson::Document>(std::move(steering_factor_array_doc));
            pushToDataChannel("Vehicle.ADS.Planning.SteeringFactor.Factors", doc_ptr, ms_since_epoch);
        }
    }
}



void DDSProvider::handleVelocityFactorArray(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::VelocityFactorArray sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "VelocityFactorArray";
            rapidjson::Document velocity_factor_array_doc;
            velocity_factor_array_doc.SetArray();
            rapidjson::Document::AllocatorType& allocator = velocity_factor_array_doc.GetAllocator();
            long long ms_since_epoch = getTimestampMs(sample.header());
            std::vector<autoware_adapi_v1_msgs::msg::VelocityFactor> velocity_factors = sample.factors();
            uint32_t count = velocity_factors.size();
            pushToDataChannel("Vehicle.ADS.Planning.VelocityFactor.Count", count, ms_since_epoch);
            LOG_DEBUG << "Vehicle.ADS.Planning.VelocityFactor.Count:" << count;

            for (const auto& factor : velocity_factors)
            {
                rapidjson::Value factor_val(rapidjson::kObjectType);

                factor_val.AddMember("pose", serializePose(factor.pose(), allocator), allocator);
                factor_val.AddMember("distance", factor.distance(), allocator);

                uint16_t status = factor.status();

                if(status == autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::APPROACHING) {
                    factor_val.AddMember("status", rapidjson::Value("APPROACHING", allocator), allocator);
                } else if(status == autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::STOPPED) {
                    factor_val.AddMember("status", rapidjson::Value("STOPPED", allocator), allocator);
                } else {// autoware_adapi_v1_msgs::msg::VelocityFactor_Constants::UNKNOWN
                    factor_val.AddMember("status", rapidjson::Value("UNKNOWN", allocator), allocator);
                }

                factor_val.AddMember("behavior", rapidjson::Value(factor.behavior().c_str(), allocator), allocator);
                factor_val.AddMember("sequence", rapidjson::Value(factor.sequence_().c_str(), allocator), allocator);
                factor_val.AddMember("detail", rapidjson::Value(factor.detail().c_str(), allocator), allocator);

                rapidjson::Value cooperation_status_array = serializeCooperationStatusArray(factor.cooperation(), allocator);
                factor_val.AddMember("cooperation", cooperation_status_array, allocator);

                velocity_factor_array_doc.PushBack(factor_val, allocator);
            }

            auto doc_ptr = std::make_shared<rapidjson::Document>(std::move(velocity_factor_array_doc));
            pushToDataChannel("Vehicle.ADS.Planning.VelocityFactor.Factors", doc_ptr, ms_since_epoch);
        }
    }
}

void DDSProvider::handleGnssOrientation(DataReader* reader)
{
    SampleInfo info;
    autoware_sensing_msgs::msg::GnssInsOrientationStamped sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            rapidjson::Document gnss_doc;
            gnss_doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = gnss_doc.GetAllocator();

            rapidjson::Value orient_val(rapidjson::kObjectType);
            orient_val.AddMember("x", rapidjson::Value(std::to_string(sample.orientation().orientation().x()).c_str(), allocator), allocator);
            orient_val.AddMember("y", rapidjson::Value(std::to_string(sample.orientation().orientation().y()).c_str(), allocator), allocator);
            orient_val.AddMember("z", rapidjson::Value(std::to_string(sample.orientation().orientation().z()).c_str(), allocator), allocator);
            orient_val.AddMember("w", rapidjson::Value(std::to_string(sample.orientation().orientation().w()).c_str(), allocator), allocator);
            gnss_doc.AddMember("orientation", orient_val, allocator);

            gnss_doc.AddMember("rmse_rotation_x", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_x()).c_str(), allocator), allocator);
            gnss_doc.AddMember("rmse_rotation_y", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_y()).c_str(), allocator), allocator);
            gnss_doc.AddMember("rmse_rotation_z", rapidjson::Value(std::to_string(sample.orientation().rmse_rotation_z()).c_str(), allocator), allocator);

            auto doc_ptr = std::make_shared<rapidjson::Document>(std::move(gnss_doc));
            
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc_ptr->Accept(writer);
            LOG_INFO << "DDSProvider: Pushing to channel -> Path: Vehicle.ADS.GnssInsOrientation, Value: " << buffer.GetString();

            pushToDataChannel("Vehicle.ADS.GnssInsOrientation", doc_ptr);
        }
    }
}

void DDSProvider::handleTrajectory(DataReader* reader)
{
    SampleInfo info;
    autoware_planning_msgs::msg::Trajectory sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            try {
                rapidjson::Document trajectory_doc;
                trajectory_doc.SetArray();
                rapidjson::Document::AllocatorType& allocator = trajectory_doc.GetAllocator();
                long long ms_since_epoch = getTimestampMs(sample.header());
                const std::vector<autoware_planning_msgs::msg::TrajectoryPoint> trajectory_points = sample.points();
                size_t count = trajectory_points.size();
                pushToDataChannel("Vehicle.ADS.Planning.Trajectory.PointCount", std::any(count), ms_since_epoch);
                LOG_INFO << "DDSProvider: Pushing to channel -> Path: Vehicle.ADS.Planning.Trajectory.PointCount, Value: " << count;

                for (const auto& p : trajectory_points) {
                    rapidjson::Document point_doc;
                    point_doc.SetObject();
                    
                    rapidjson::Value pose_val(rapidjson::kObjectType);
                    rapidjson::Value pos_val(rapidjson::kObjectType);
                    pos_val.AddMember("x", rapidjson::Value(std::to_string(p.pose().position().x()).c_str(), allocator), allocator);
                    pos_val.AddMember("y", rapidjson::Value(std::to_string(p.pose().position().y()).c_str(), allocator), allocator);
                    pos_val.AddMember("z", rapidjson::Value(std::to_string(p.pose().position().z()).c_str(), allocator), allocator);
                    pose_val.AddMember("position", pos_val, allocator);
                    
                    rapidjson::Value orient_val(rapidjson::kObjectType);
                    orient_val.AddMember("x", rapidjson::Value(std::to_string(p.pose().orientation().x()).c_str(), allocator), allocator);
                    orient_val.AddMember("y", rapidjson::Value(std::to_string(p.pose().orientation().y()).c_str(), allocator), allocator);
                    orient_val.AddMember("z", rapidjson::Value(std::to_string(p.pose().orientation().z()).c_str(), allocator), allocator);
                    orient_val.AddMember("w", rapidjson::Value(std::to_string(p.pose().orientation().w()).c_str(), allocator), allocator);
                    pose_val.AddMember("orientation", orient_val, allocator);

                    point_doc.AddMember("pose", pose_val, allocator);

                    auto time_from_start = p.time_from_start();
                    rapidjson::Value time_val(rapidjson::kObjectType);
                    time_val.AddMember("sec", rapidjson::Value(std::to_string(time_from_start.sec()).c_str(), allocator), allocator);
                    time_val.AddMember("nanosec", rapidjson::Value(std::to_string(time_from_start.nanosec()).c_str(), allocator), allocator);
                    point_doc.AddMember("time_from_start", time_val, allocator);

                    point_doc.AddMember("longitudinal_velocity_mps", rapidjson::Value(std::to_string(p.longitudinal_velocity_mps()).c_str(), allocator), allocator);
                    point_doc.AddMember("lateral_velocity_mps", rapidjson::Value(std::to_string(p.lateral_velocity_mps()).c_str(), allocator), allocator);
                    point_doc.AddMember("acceleration_mps2", rapidjson::Value(std::to_string(p.acceleration_mps2()).c_str(), allocator), allocator);
                    point_doc.AddMember("heading_rate_rps", rapidjson::Value(std::to_string(p.heading_rate_rps()).c_str(), allocator), allocator);
                    point_doc.AddMember("front_wheel_angle_rad", rapidjson::Value(std::to_string(p.front_wheel_angle_rad()).c_str(), allocator), allocator);
                    point_doc.AddMember("rear_wheel_angle_rad", rapidjson::Value(std::to_string(p.rear_wheel_angle_rad()).c_str(), allocator), allocator);

                    trajectory_doc.PushBack(point_doc, allocator);
                }

                auto doc_ptr = std::make_shared<rapidjson::Document>(std::move(trajectory_doc));

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc_ptr->Accept(writer);
                LOG_INFO << "DDSProvider: Pushing to channel -> Path: Vehicle.ADS.Planning.Trajectory.Points, Value: " << buffer.GetString();

                pushToDataChannel("Vehicle.ADS.Planning.Trajectory.Points", doc_ptr, ms_since_epoch);
            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in trajectory parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleVehicleKinematics(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::VehicleKinematics sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "VehicleKinematics";
            try {
                rapidjson::Document doc;
                doc.SetObject();
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                // geographic_pose
                double latitude = sample.geographic_pose().position().latitude();
                double longitude = sample.geographic_pose().position().longitude();
                double altitude = sample.geographic_pose().position().altitude();

                long long ms_since_epoch = getTimestampMs(sample.geographic_pose().header());

                pushToDataChannel("Vehicle.CurrentLocation.Latitude", std::to_string(latitude), ms_since_epoch);
                LOG_DEBUG << "Vehicle.CurrentLocation.Latitude:" << latitude;

                pushToDataChannel("Vehicle.CurrentLocation.Longitude", std::to_string(longitude), ms_since_epoch);
                LOG_DEBUG << "Vehicle.CurrentLocation.Longitude:" << longitude;

                pushToDataChannel("Vehicle.CurrentLocation.Altitude", std::to_string(altitude), ms_since_epoch);
                LOG_DEBUG << "Vehicle.CurrentLocation.Altitude:" << altitude;


                // geographic_pose (개별 저장 + doc 포함)
                rapidjson::Value geographic_pose_val = serializeGeoPoint(sample.geographic_pose().position(), allocator);
                pushToDataChannel("Vehicle.ADS.VehicleKinematics.GeographicPose", toDocumentPtr(geographic_pose_val, allocator), ms_since_epoch);
                doc.AddMember("geographic_pose", geographic_pose_val, allocator);

                // pose (heading 계산 + 개별 저장 + doc 포함)
                double x = sample.pose().pose_().pose_().orientation().x();
                double y = sample.pose().pose_().pose_().orientation().y();
                double z = sample.pose().pose_().pose_().orientation().z();
                double w = sample.pose().pose_().pose_().orientation().w();

                long long pose_ms_since_epoch = getTimestampMs(sample.pose().header());

                // Quaternion to Yaw (Heading)
                double siny_cosp = 2.0 * (w * z + x * y);
                double cosy_cosp = 1.0 - 2.0 * (y * y + z * z);
                double heading_rad = std::atan2(siny_cosp, cosy_cosp);
                double heading_deg = heading_rad * 180.0 / 3.14159265358979323846;
                if (heading_deg < 0.0) {
                    heading_deg += 360.0;
                }

                pushToDataChannel("Vehicle.CurrentLocation.Heading", std::to_string(heading_deg), pose_ms_since_epoch);
                LOG_DEBUG << "Vehicle.CurrentLocation.Heading:" << heading_deg;

                rapidjson::Value pose_val = serializePoseWithCovariance(sample.pose().pose_(), allocator);
                pushToDataChannel("Vehicle.ADS.VehicleKinematics.Pose", toDocumentPtr(pose_val, allocator), pose_ms_since_epoch);
                doc.AddMember("pose", pose_val, allocator);

                // twist (개별 저장 + doc 포함)
                long long twist_ms = getTimestampMs(sample.twist().header());
                rapidjson::Value twist_val = serializeTwistWithCovariance(sample.twist().twist_(), allocator);
                pushToDataChannel("Vehicle.ADS.VehicleKinematics.Twist", toDocumentPtr(twist_val, allocator), twist_ms);
                doc.AddMember("twist", twist_val, allocator);

                // accel (개별 저장 + doc 포함)
                long long accel_ms = getTimestampMs(sample.accel().header());
                rapidjson::Value accel_val = serializeAccelWithCovariance(sample.accel().accel_(), allocator);
                pushToDataChannel("Vehicle.ADS.VehicleKinematics.Accel", toDocumentPtr(accel_val, allocator), accel_ms);
                doc.AddMember("accel", accel_val, allocator);

                // Speed (m/s to km/h)
                double speed_kmh = sample.twist().twist_().twist_().linear().x() * 3.6;
                pushToDataChannel("Vehicle.Speed", std::to_string(speed_kmh), twist_ms);
                LOG_DEBUG << "Vehicle.Speed:" << speed_kmh;

                // Angular Velocity (rad/s to deg/s)
                const double RAD_TO_DEG = 180.0 / 3.14159265358979323846;
                pushToDataChannel("Vehicle.AngularVelocity.Roll", std::to_string(sample.twist().twist_().twist_().angular().x() * RAD_TO_DEG), twist_ms);
                pushToDataChannel("Vehicle.AngularVelocity.Pitch", std::to_string(sample.twist().twist_().twist_().angular().y() * RAD_TO_DEG), twist_ms);
                pushToDataChannel("Vehicle.AngularVelocity.Yaw", std::to_string(sample.twist().twist_().twist_().angular().z() * RAD_TO_DEG), twist_ms);

                // Acceleration (m/s^2)
                pushToDataChannel("Vehicle.Acceleration.Longitudinal", std::to_string(sample.accel().accel_().accel_().linear().x()), accel_ms);
                pushToDataChannel("Vehicle.Acceleration.Lateral", std::to_string(sample.accel().accel_().accel_().linear().y()), accel_ms);
                pushToDataChannel("Vehicle.Acceleration.Vertical", std::to_string(sample.accel().accel_().accel_().linear().z()), accel_ms);

                // Entire kinematics document
                auto doc_ptr = std::make_shared<rapidjson::Document>(std::move(doc));
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc_ptr->Accept(writer);
                LOG_INFO << "DDSProvider: Pushing to channel -> Path: Vehicle.ADS.VehicleKinematics, Value: " << buffer.GetString();

                pushToDataChannel("Vehicle.ADS.VehicleKinematics", doc_ptr, ms_since_epoch);
            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in VehicleKinematics parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleVehicleStatus(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::VehicleStatus sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "VehicleStatus";
            try {
                uint8_t turn = sample.turn_indicators().status();
                uint8_t gear = sample.gear_().status();
                uint8_t hazard = sample.hazard_lights().status();

                long long ms_since_epoch = getTimestampMs(sample.stamp());

                if (gear != autoware_adapi_v1_msgs::msg::Gear_Constants::UNKNOWN) {
                    std::string gear_val;
                    if(gear == autoware_adapi_v1_msgs::msg::Gear_Constants::DRIVE)
                        gear_val = "127";
                    else if(gear == autoware_adapi_v1_msgs::msg::Gear_Constants::REVERSE)
                        gear_val = "-1";
                    else if(gear == autoware_adapi_v1_msgs::msg::Gear_Constants::PARK)
                        gear_val = "126";
                    else if(gear == autoware_adapi_v1_msgs::msg::Gear_Constants::LOW)
                        gear_val = "2";
                    else  //autoware_adapi_v1_msgs::msg::Gear_Constants::NEUTRAL
                        gear_val = "0"; 

                    pushToDataChannel("Vehicle.Powertrain.Transmission.SelectedGear", gear_val, ms_since_epoch);
                    LOG_DEBUG << "Vehicle.Powertrain.Transmission.SelectedGear:" << gear_val;
                }

                if (turn != autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::UNKNOWN) {
                    std::string left_val = "false";
                    std::string right_val = "false";
                    if(turn == autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::LEFT) {
                        left_val = "true";
                    }
                    else if(turn == autoware_adapi_v1_msgs::msg::TurnIndicators_Constants::RIGHT) {
                        right_val = "true";
                    }
                    pushToDataChannel("Vehicle.Body.Lights.DirectionIndicator.Left.IsSignaling", left_val, ms_since_epoch);
                    pushToDataChannel("Vehicle.Body.Lights.DirectionIndicator.Right.IsSignaling", right_val, ms_since_epoch);
                    LOG_DEBUG << "DirectionIndicator: Left=" << left_val << ", Right=" << right_val;
                }

                if (hazard != autoware_adapi_v1_msgs::msg::HazardLights_Constants::UNKNOWN) {
                    std::string hazard_val = "false";
                    if(hazard == autoware_adapi_v1_msgs::msg::HazardLights_Constants::ENABLE) {
                        hazard_val = "true";
                    }
                    pushToDataChannel("Vehicle.Body.Lights.Hazard.IsSignaling", hazard_val, ms_since_epoch);
                    LOG_DEBUG << "Vehicle.Body.Lights.Hazard.IsSignaling:" << hazard_val;
                }

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in VehicleStatus parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleRouteState(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::RouteState sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "RouteState";
            try {
                uint16_t state = sample.state();
                long long ms_since_epoch = getTimestampMs(sample.stamp());

                std::string state_val;
                if(state == autoware_adapi_v1_msgs::msg::RouteState_Constants::UNSET)
                    state_val = "UNSET";
                else if(state == autoware_adapi_v1_msgs::msg::RouteState_Constants::SETUP)
                    state_val = "SETUP";
                else if(state == autoware_adapi_v1_msgs::msg::RouteState_Constants::ARRIVED)
                    state_val = "ARRIVED";
                else if(state == autoware_adapi_v1_msgs::msg::RouteState_Constants::CHANGING)
                    state_val = "CHANGING";
                else  //autoware_adapi_v1_msgs::msg::RouteState_Constants::UNKNOWN
                    state_val = "UNKNOWN"; 

                pushToDataChannel("Vehicle.ADS.Route.RouteState", state_val, ms_since_epoch);
                LOG_DEBUG << "Vehicle.ADS.Route.RouteState:" << state_val;

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in RouteState parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleRoute(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::Route sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "Route";
            try {
                long long ms_since_epoch = getTimestampMs(sample.header());

                if (sample.data().empty()) return;

                const auto& route_data = sample.data()[0];

                rapidjson::Document doc;
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                // Start Pose
                rapidjson::Value start_val = serializePose(route_data.start(), allocator);
                pushToDataChannel("Vehicle.ADS.Route.Start", toDocumentPtr(start_val, allocator), ms_since_epoch);

                // Goal Pose
                rapidjson::Value goal_val = serializePose(route_data.goal(), allocator);
                pushToDataChannel("Vehicle.ADS.Route.Goal", toDocumentPtr(goal_val, allocator), ms_since_epoch);

                // Segments
                rapidjson::Document seg_doc;
                seg_doc.SetArray();
                rapidjson::Document::AllocatorType& alloc = seg_doc.GetAllocator();
                for (const auto& seg : route_data.segments()) {
                    rapidjson::Value seg_obj(rapidjson::kObjectType);
                    seg_obj.AddMember("id", static_cast<int64_t>(seg.preferred().id()), alloc);
                    seg_obj.AddMember("type", rapidjson::Value(seg.preferred().type().c_str(), alloc), alloc);
                    seg_obj.AddMember("alt_count", static_cast<int>(seg.alternatives().size()), alloc);
                    seg_doc.PushBack(seg_obj, alloc);
                }

                auto seg_ptr = std::make_shared<rapidjson::Document>(std::move(seg_doc));
                pushToDataChannel("Vehicle.ADS.Route.Segments", seg_ptr, ms_since_epoch);

                LOG_DEBUG << "Route updated. Start and Goal poses saved.";

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in Route parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleMrmState(DataReader* reader)
{
    SampleInfo info;
    autoware_adapi_v1_msgs::msg::MrmState sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "MrmState";
            try {
                uint16_t state = sample.state();
                uint16_t behavior = sample.behavior();
                long long ms_since_epoch = getTimestampMs(sample.stamp());

                std::string state_val;
                if(state == autoware_adapi_v1_msgs::msg::MrmState_Constants::NORMAL)
                    state_val = "NORMAL";
                else if(state == autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_OPERATING)
                    state_val = "MRM_OPERATING";
                else if(state == autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_SUCCEEDED)
                    state_val = "MRM_SUCCEEDED";
                else if(state == autoware_adapi_v1_msgs::msg::MrmState_Constants::MRM_FAILED)
                    state_val = "MRM_FAILED";
                else  //autoware_adapi_v1_msgs::msg::MrmState_Constants::UNKNOWN
                    state_val = "UNKNOWN"; 

                pushToDataChannel("Vehicle.ADS.Safety.MRM.Status", state_val, ms_since_epoch);

                std::string behavior_val;
                if(behavior == autoware_adapi_v1_msgs::msg::MrmState_Constants::NONE)
                    behavior_val = "NONE";
                else if(behavior == autoware_adapi_v1_msgs::msg::MrmState_Constants::EMERGENCY_STOP)
                    behavior_val = "EMERGENCY_STOP";
                else if(behavior == autoware_adapi_v1_msgs::msg::MrmState_Constants::COMFORTABLE_STOP)
                    behavior_val = "COMFORTABLE_STOP";
                else if(behavior == autoware_adapi_v1_msgs::msg::MrmState_Constants::PULL_OVER)
                    behavior_val = "PULL_OVER";
                else  //autoware_adapi_v1_msgs::msg::MrmState_Constants::UNKNOWN
                    behavior_val = "UNKNOWN"; 

                pushToDataChannel("Vehicle.ADS.Safety.MRM.Behavior", behavior_val, ms_since_epoch);
                LOG_DEBUG << "MRM Status: " << state_val << ", Behavior: " << behavior_val;

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in MrmState parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleEmergency(DataReader* reader)
{
    SampleInfo info;
    tier4_external_api_msgs::msg::Emergency sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "Emergency";
            try {
                long long ms_since_epoch = getTimestampMs(sample.stamp());
                bool emergency = sample.emergency();

                pushToDataChannel("Vehicle.ADS.IsEmergency", emergency ? "true" : "false", ms_since_epoch);
                LOG_DEBUG << "Vehicle.ADS.IsEmergency:" << emergency;

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in Emergency parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleHazardStatus(DataReader* reader)
{
    SampleInfo info;
    autoware_system_msgs::msg::HazardStatusStamped sample;
    if (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_DEBUG << "Hazard Status";
            try {
                long long ms_since_epoch = getTimestampMs(sample.stamp());
                autoware_system_msgs::msg::HazardStatus hs = sample.status();
                uint8_t level = hs.level();
                bool emergency = hs.emergency();
                std::string level_str = "";

                if (level == autoware_system_msgs::msg::HazardStatus_Constants::NO_FAULT)
                    level_str = "NO_FAULT";
                else if (level == autoware_system_msgs::msg::HazardStatus_Constants::SAFE_FAULT)
                    level_str = "SAFE_FAULT";
                else if (level == autoware_system_msgs::msg::HazardStatus_Constants::LATENT_FAULT)
                    level_str = "LATENT_FAULT";
                else if (level == autoware_system_msgs::msg::HazardStatus_Constants::SINGLE_POINT_FAULT)
                    level_str = "SINGLE_POINT_FAULT";
                else
                    level_str = "UNKNOWN";

                pushToDataChannel("Vehicle.ADS.Safety.HazardStatus.Level", level_str, ms_since_epoch);
                pushToDataChannel("Vehicle.ADS.Safety.HazardStatus.IsEmergency", emergency ? "true" : "false", ms_since_epoch);
                LOG_DEBUG << "Hazard Level: " << level_str << ", Emergency: " << emergency;

                // 공통 헬퍼를 사용하여 각 fault 유형의 diagnostics를 직렬화 및 푸시
                auto pushDiagFault = [&](const std::vector<diagnostic_msgs::msg::DiagnosticStatus>& diag_list,
                                        const std::string& channel_path) {
                    auto doc_ptr = serializeDiagnostics(diag_list);
                    rapidjson::StringBuffer buf;
                    rapidjson::Writer<rapidjson::StringBuffer> w(buf);
                    doc_ptr->Accept(w);
                    LOG_INFO << "DDSProvider: Pushing to channel -> Path: " << channel_path << ", Value: " << buf.GetString();
                    pushToDataChannel(channel_path, doc_ptr, ms_since_epoch);
                };

                pushDiagFault(hs.diag_no_fault(),           "Vehicle.ADS.Safety.HazardStatus.NoFault");
                pushDiagFault(hs.diag_safe_fault(),         "Vehicle.ADS.Safety.HazardStatus.SafeFault");
                pushDiagFault(hs.diag_latent_fault(),       "Vehicle.ADS.Safety.HazardStatus.LatentFault");
                pushDiagFault(hs.diag_single_point_fault(), "Vehicle.ADS.Safety.HazardStatus.SinglePointFault");
                

            } catch (const std::exception& e) {
                LOG_ERROR << "Exception in HazardStatus parsing: " << e.what();
            }
        }
    }
}

void DDSProvider::handleOperationModeAvailability(
    eprosima::fastdds::dds::DataReader* reader)
{
    tier4_system_msgs::msg::OperationModeAvailability sample;
    SampleInfo info;

    while (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_INFO << "DDSProvider: Received OperationModeAvailability";

            long long ms_since_epoch = getTimestampMs(sample.stamp());
            bool stop = sample.stop();
            bool autonomous = sample.autonomous();
            bool local = sample.is_local();
            bool remote = sample.remote();
            bool emergency_stop = sample.emergency_stop();
            bool comfortable_stop = sample.comfortable_stop();
            bool pull_over = sample.pull_over();

            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsStop", stop ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsAutonomous", autonomous ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsLocal", local ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsRemote", remote ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsEmergencyStop", emergency_stop ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsComfortableStop", comfortable_stop ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.Availability.IsPullOver", pull_over, ms_since_epoch);
        }
    }
}

void DDSProvider::handleOperationModeState(
    eprosima::fastdds::dds::DataReader* reader)
{
    autoware_adapi_v1_msgs::msg::OperationModeState sample;
    SampleInfo info;

    while (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_INFO << "DDSProvider: Received OperationModeState";

            long long ms_since_epoch = getTimestampMs(sample.stamp());
            uint8_t mode = sample.mode();
            bool is_autoware_control_enabled = sample.is_autoware_control_enabled();
            bool is_in_transition = sample.is_in_transition();
            
            std::string mode_str;
            switch (mode) {
                case autoware_adapi_v1_msgs::msg::OperationModeState_Constants::STOP:
                    mode_str = "STOP";
                    break;
                case autoware_adapi_v1_msgs::msg::OperationModeState_Constants::AUTONOMOUS:
                    mode_str = "AUTONOMOUS";
                    break;
                case autoware_adapi_v1_msgs::msg::OperationModeState_Constants::LOCAL_:
                    mode_str = "LOCAL";
                    break;
                case autoware_adapi_v1_msgs::msg::OperationModeState_Constants::REMOTE:
                    mode_str = "REMOTE";
                    break;
                default:
                    mode_str = "UNKNOWN";
                    break;
            }

            pushToDataChannel("Vehicle.ADS.OperationMode.State.Mode", mode_str, ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.State.IsAutowareControlEnabled", is_autoware_control_enabled ? "true" : "false", ms_since_epoch);
            pushToDataChannel("Vehicle.ADS.OperationMode.State.IsInTransition", is_in_transition ? "true" : "false", ms_since_epoch);
        }
    }
}

void DDSProvider::handleDynamicObjectArray(
    eprosima::fastdds::dds::DataReader* reader)
{
    autoware_adapi_v1_msgs::msg::DynamicObjectArray sample;
    SampleInfo info;

    while (reader->take_next_sample(&sample, &info) == RETCODE_OK)
    {
        if (info.valid_data)
        {
            LOG_INFO << "DDSProvider: Received DynamicObjectArray";

            long long ms_since_epoch = getTimestampMs(sample.header());
            auto objects = sample.objects();
            size_t object_count = objects.size();

            pushToDataChannel("Vehicle.ADS.Perception.DynamicObject.ObjectCount", std::any(object_count), ms_since_epoch);
            LOG_INFO << "DDSProvider: DynamicObjectArray object count: " << object_count;
            



            rapidjson::Document dynamicObjects;
            dynamicObjects.SetArray();

            for (auto obj: objects) {

                std::array<uint8_t, 16> uuid = obj.id().uuid();
                double existence_probability = obj.existence_probability();
                std::vector<autoware_adapi_v1_msgs::msg::ObjectClassification> classification = obj.classification();
                autoware_adapi_v1_msgs::msg::DynamicObjectKinematics kinematics = obj.kinematics();
                shape_msgs::msg::SolidPrimitive shape = obj.shape();


                rapidjson::Value obj_doc(rapidjson::kObjectType);


                rapidjson::Value uuid_array(rapidjson::kArrayType);
                for(const auto& u : uuid){
                    uuid_array.PushBack(u, dynamicObjects.GetAllocator());
                }

                obj_doc.AddMember("id", uuid_array, dynamicObjects.GetAllocator());
                obj_doc.AddMember("existence_probability", rapidjson::Value(existence_probability), dynamicObjects.GetAllocator());


                rapidjson::Value classification_array(rapidjson::kArrayType);
                for(const auto& c : classification){
                    uint8_t label = c.label();
                    double probability = c.probability();

                    std::string label_str;
                    if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::CAR){
                        label_str = "CAR";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRUCK){
                        label_str = "TRUCK";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BUS){
                        label_str = "BUS";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::TRAILER){
                        label_str = "TRAILER";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::MOTORCYCLE){
                        label_str = "MOTORCYCLE";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::BICYCLE){
                        label_str = "BICYCLE";
                    }else if(label == autoware_adapi_v1_msgs::msg::ObjectClassification_Constants::PEDESTRIAN){
                        label_str = "PEDESTRIAN";
                    }else{
                        label_str = "UNKNOWN";
                    }

                    rapidjson::Value c_value(rapidjson::kObjectType);
                    c_value.AddMember("label", rapidjson::Value(label_str.c_str(), dynamicObjects.GetAllocator()), dynamicObjects.GetAllocator());
                    c_value.AddMember("probability", rapidjson::Value(probability), dynamicObjects.GetAllocator());

                    classification_array.PushBack(c_value, dynamicObjects.GetAllocator());
    
                }
                obj_doc.AddMember("classification", classification_array, dynamicObjects.GetAllocator());


                rapidjson::Value kinematics_value = serializeKinematics(kinematics, dynamicObjects.GetAllocator());
                obj_doc.AddMember("kinematics", kinematics_value, dynamicObjects.GetAllocator());

                rapidjson::Value shape_value = serializeSolidPrimitive(shape, dynamicObjects.GetAllocator());
                obj_doc.AddMember("shape", shape_value, dynamicObjects.GetAllocator());

                dynamicObjects.PushBack(obj_doc, dynamicObjects.GetAllocator());
            }


            auto dynamicObjects_ptr = std::make_shared<rapidjson::Document>(std::move(dynamicObjects));

            rapidjson::StringBuffer buffer_dynamicObjects;
            rapidjson::Writer<rapidjson::StringBuffer> writer_dynamicObjects(buffer_dynamicObjects);
            dynamicObjects_ptr->Accept(writer_dynamicObjects);
            LOG_INFO << "DDSProvider: Pushing to channel -> Path: Vehicle.ADS.Perception.DynamicObject.Objects, Value: " << buffer_dynamicObjects.GetString();

            pushToDataChannel("Vehicle.ADS.Perception.DynamicObject.Objects", dynamicObjects_ptr, ms_since_epoch);
        }
    }
}

std::string DDSProvider::getName() const
{
    return "DDSProvider";
}

void DDSProvider::setDataChannel(const std::shared_ptr<ThreadSafeQueue<VehicleData>>& channel)
{
    data_channel_ = channel;
}

void DDSProvider::setValue(const std::string& path, const std::any& value)
{
    // This provider is currently only for subscribing, so this is a no-op for now.
    // We only write to the VehicleData topic for now.
    if (vehicle_data_writer_ != nullptr)
    {
        VehicleDataTopic sample;
        sample.path(path);
        sample.value(any_to_string(value)); // Using helper to convert any to string
        vehicle_data_writer_->write(&sample);
        LOG_INFO << "DDSProvider: Published data for path " << path;
    }
}

void DDSProvider::setVssTree(std::shared_ptr<viss::VSSTree> vss_tree)
{
    vss_tree_ = vss_tree;
}
