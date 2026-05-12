// OtaBridge: forwards OtaManager bare-DDS topics to ROS-canonical
// `rt/...` topics so rosbridge_server / rclcpp / rclpy can match them.
//
// The bare-DDS IDL produces a TypeObject named "<ns>::<Struct>" (e.g.
// "otamanager::GetAppList") with field names like "id", which mismatches
// what rmw_fastrtps generates from rosidl ("<ns>::msg::dds_::<Struct>_"
// with field names like "id_"). Adding rt/ topics + register_type
// aliases inside OtaManager isn't enough either — the underlying
// TypeObject embeds the IDL type name and field names so XTypes type
// hash differs.
//
// Diagnostics' DDSProtocol works around this by loading the IDL at
// runtime via FastDDS's dynamic-IDL parser and registering the parsed
// type under BOTH names on a single participant. Both registrations
// share the same TypeObject — wire matching is name-only — so peers on
// either side (raw IDL or ROS-canonical) match without changes.
//
// This class applies the same trick as a forwarder for OtaManager:
//   request  : SubReader on rt/...  (ROS canonical)  →  PubWriter on /mmf/... (raw IDL)
//   response : SubReader on /mmf/... (raw IDL)       →  PubWriter on rt/...  (ROS canonical)

#ifndef OTA_BRIDGE_H_
#define OTA_BRIDGE_H_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>

#include <memory>
#include <string>
#include <vector>

// One bidirectional bridge configuration.
//
// ros_to_raw=true  → SubReader on ros_topic (ros_type), PubWriter on raw_topic (raw_type).
//                    Used for ros_monitor → OtaManager (request / registration).
// ros_to_raw=false → SubReader on raw_topic (raw_type), PubWriter on ros_topic (ros_type).
//                    Used for OtaManager → ros_monitor (response / notify).
struct PairConfig {
    std::string struct_name;   // e.g. "GetAppList"
    std::string idl_file;      // path under idl_root, e.g. "ota_mgr/OtamActorServiceMsg.idl"
    std::string raw_topic;     // bare DDS topic name (OtaManager-facing)
    std::string ros_topic;     // rt/-prefixed topic name (rosbridge-facing)
    std::string raw_type;      // bare IDL type name, e.g. "otamanager::GetAppList"
    std::string ros_type;      // ROS-canonical alias, e.g. "otamanager::msg::dds_::GetAppList_"
    bool ros_to_raw;           // direction of forwarding
};

// FastDDS DataReader listener that hands every received sample off to a
// matched DataWriter on the opposite side of the bridge. The dynamic
// type is owned by OtaBridge; the listener keeps a non-owning view.
class ForwardListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    ForwardListener(eprosima::fastdds::dds::DataWriter* writer,
                    eprosima::fastdds::dds::DynamicType::_ref_type type,
                    std::string tag);

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

private:
    eprosima::fastdds::dds::DataWriter* writer_;
    eprosima::fastdds::dds::DynamicType::_ref_type type_;
    std::string tag_;
};

// Owns a single FastDDS participant + publisher/subscriber and a set of
// per-topic forwarding pairs. Call Initialize() once, then AddPair() for
// every PairConfig. Forwarding starts asynchronously as soon as readers
// are created — the caller's main thread can just sleep.
class OtaBridge {
public:
    bool Initialize(const std::string& idl_root);
    bool AddPair(const PairConfig& cfg);

private:
    std::string idl_root_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher*         publisher_   = nullptr;
    eprosima::fastdds::dds::Subscriber*        subscriber_  = nullptr;
    std::vector<std::unique_ptr<ForwardListener>> listeners_;
};

#endif  // OTA_BRIDGE_H_
