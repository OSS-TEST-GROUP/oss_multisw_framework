// ota_dds_bridge: forwards OtaManager bare-DDS topics to ROS-canonical
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
// This binary applies the same trick as a forwarder for OtaManager:
//   request  : SubReader on rt/...  (ROS canonical)  →  PubWriter on /mmf/... (raw IDL)
//   response : SubReader on /mmf/... (raw IDL)       →  PubWriter on rt/...  (ROS canonical)
//
// Start with GetAppList only as proof-of-concept; expand to other OTA
// topics once round-trip is confirmed.

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

struct PairConfig {
    std::string struct_name;   // e.g. "GetAppList"
    std::string idl_file;      // path under idl_root, e.g. "ota_mgr/OtamClientServiceMsg.idl"
    std::string raw_topic;     // bare DDS topic name (OtaManager-facing)
    std::string ros_topic;     // rt/-prefixed topic name (rosbridge-facing)
    std::string raw_type;      // bare IDL type name, e.g. "otamanager::GetAppList"
    std::string ros_type;      // ROS-canonical alias, e.g. "otamanager::msg::dds_::GetAppList_"
    bool ros_to_raw;           // direction of forwarding
};

class ForwardListener : public DataReaderListener {
public:
    ForwardListener(DataWriter* writer, DynamicType::_ref_type type, std::string tag)
        : writer_(writer), type_(type), tag_(std::move(tag)) {}

    void on_data_available(DataReader* reader) override {
        DynamicData::_ref_type data = DynamicDataFactory::get_instance()->create_data(type_);
        SampleInfo info;
        if (reader->take_next_sample(&data, &info) != RETCODE_OK) return;
        if (!info.valid_data) return;
        if (writer_->write(&data) == RETCODE_OK) {
            std::cout << "[bridge] " << tag_ << " forwarded\n";
        } else {
            std::cerr << "[bridge] " << tag_ << " write FAILED\n";
        }
    }

private:
    DataWriter* writer_;
    DynamicType::_ref_type type_;
    std::string tag_;
};

class OtaBridge {
public:
    bool Initialize(const std::string& idl_root) {
        idl_root_ = idl_root;
        auto* factory = DomainParticipantFactory::get_instance();

        DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
        // No ignore_local_endpoints here: bridge per-direction only
        // creates one side (writer for ros→raw, reader for raw→ros) so
        // there is no same-participant loopback to suppress, and the
        // property has been observed to subtly affect cross-process
        // matching in some FastDDS versions.

        participant_ = factory->create_participant(0, qos);
        if (!participant_) {
            std::cerr << "[bridge] create_participant failed\n";
            return false;
        }
        publisher_  = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!publisher_ || !subscriber_) {
            std::cerr << "[bridge] create publisher/subscriber failed\n";
            return false;
        }
        return true;
    }

    bool AddPair(const PairConfig& cfg) {
        std::string idl_path = idl_root_ + "/" + cfg.idl_file;
        auto builder = DynamicTypeBuilderFactory::get_instance()
            ->create_type_w_uri(idl_path, cfg.struct_name, {idl_root_});
        if (!builder) {
            std::cerr << "[bridge] failed to parse IDL: " << idl_path
                      << " struct=" << cfg.struct_name << "\n";
            return false;
        }
        DynamicType::_ref_type dyn_type = builder->build();

        // Two separate TypeSupport instances, each registered under a
        // single name. Sharing one PubSubType across two names made
        // wire-level type matching against OtaManager's IDL-compiled
        // peer flaky in early prototyping.
        TypeSupport ts_raw(new DynamicPubSubType(dyn_type));
        ts_raw.register_type(participant_, cfg.raw_type);
        TypeSupport ts_ros(new DynamicPubSubType(dyn_type));
        ts_ros.register_type(participant_, cfg.ros_type);

        Topic* raw_topic = participant_->create_topic(cfg.raw_topic, cfg.raw_type, TOPIC_QOS_DEFAULT);
        Topic* ros_topic = participant_->create_topic(cfg.ros_topic, cfg.ros_type, TOPIC_QOS_DEFAULT);
        if (!raw_topic || !ros_topic) {
            std::cerr << "[bridge] create_topic failed for " << cfg.struct_name << "\n";
            return false;
        }

        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        // Loosen XTypes type-compatibility checks: bridge's
        // dynamic-IDL TypeObject won't byte-match OtaManager's
        // IDL-compiled TypeObject (different code paths produce
        // different member id assignments / hashes) but they
        // describe the same CDR layout — that's all we need.
        rqos.type_consistency().m_kind = ALLOW_TYPE_COERCION;
        rqos.type_consistency().m_force_type_validation = false;
        rqos.type_consistency().m_ignore_member_names = true;
        rqos.type_consistency().m_ignore_string_bounds = true;
        rqos.type_consistency().m_ignore_sequence_bounds = true;
        rqos.type_consistency().m_prevent_type_widening = false;

        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        // Force VOLATILE durability so the bridge doesn't replay
        // historical samples to late-subscribing peers. OtaManager
        // publishes responses with TRANSIENT_LOCAL, and without this
        // override probe runs would receive the cumulative backlog of
        // previous response samples on every reconnect.
        wqos.durability().kind = VOLATILE_DURABILITY_QOS;

        DataReader* reader = nullptr;
        DataWriter* writer = nullptr;
        std::string tag;
        if (cfg.ros_to_raw) {
            writer = publisher_->create_datawriter(raw_topic, wqos);
            tag = cfg.struct_name + " (ros→raw)";
            auto listener = std::make_unique<ForwardListener>(writer, dyn_type, tag);
            reader = subscriber_->create_datareader(ros_topic, rqos, listener.get());
            listeners_.push_back(std::move(listener));
        } else {
            writer = publisher_->create_datawriter(ros_topic, wqos);
            tag = cfg.struct_name + " (raw→ros)";
            auto listener = std::make_unique<ForwardListener>(writer, dyn_type, tag);
            reader = subscriber_->create_datareader(raw_topic, rqos, listener.get());
            listeners_.push_back(std::move(listener));
        }
        if (!reader || !writer) {
            std::cerr << "[bridge] reader/writer creation failed for " << cfg.struct_name << "\n";
            return false;
        }
        std::cout << "[bridge] forward " << tag << "\n";
        return true;
    }

private:
    std::string idl_root_;
    DomainParticipant* participant_ = nullptr;
    Publisher*         publisher_   = nullptr;
    Subscriber*        subscriber_  = nullptr;
    std::vector<std::unique_ptr<ForwardListener>> listeners_;
};

static std::atomic<bool> g_running{true};
static void on_sigint(int) { g_running = false; }

int main(int argc, char** argv) {
    std::string idl_root = "idl";
    if (argc > 1) idl_root = argv[1];

    OtaBridge bridge;
    if (!bridge.Initialize(idl_root)) return 1;

    // ros_to_raw=true  → subscribe rt/<topic> (ROS-canonical type) +
    //                    publish raw <topic> (IDL type).  Used for
    //                    ros_monitor → OtaManager (request/registration).
    // ros_to_raw=false → subscribe raw <topic> (IDL type) +
    //                    publish rt/<topic> (ROS-canonical type). Used
    //                    for OtaManager → ros_monitor (response/notify).
    auto add = [&](const std::string& struct_name,
                   const std::string& idl_file,
                   const std::string& topic_path,
                   bool ros_to_raw) -> bool {
        return bridge.AddPair({
            struct_name, idl_file,
            topic_path,
            "rt" + topic_path,
            "otamanager::" + struct_name,
            "otamanager::msg::dds_::" + struct_name + "_",
            ros_to_raw,
        });
    };

    const std::string actor_idl  = "ota_mgr/OtamActorServiceMsg.idl";
    const std::string client_idl = "ota_mgr/OtamClientServiceMsg.idl";
    bool ok = true;

    // ── otaactor_service: 5 request/response pairs (10 topics) ─────────────
    const std::string ap = "/mmf/otamanager/otaactor_service/";
    ok &= add("GetAppList",                   actor_idl, ap + "GetAppList",                   /*ros→raw*/true);
    ok &= add("GetAppListResponse",           actor_idl, ap + "GetAppListResponse",           /*raw→ros*/false);
    ok &= add("GetAppUpdateSettings",         actor_idl, ap + "GetAppUpdateSettings",         true);
    ok &= add("GetAppUpdateSettingsResponse", actor_idl, ap + "GetAppUpdateSettingsResponse", false);
    ok &= add("SetAppUpdateSettings",         actor_idl, ap + "SetAppUpdateSettings",         true);
    ok &= add("SetAppUpdateSettingsResponse", actor_idl, ap + "SetAppUpdateSettingsResponse", false);
    ok &= add("GetAppUpdateState",            actor_idl, ap + "GetAppUpdateState",            true);
    ok &= add("GetAppUpdateStateResponse",    actor_idl, ap + "GetAppUpdateStateResponse",    false);
    ok &= add("SetAppUpdate",                 actor_idl, ap + "SetAppUpdate",                 true);
    ok &= add("SetAppUpdateResponse",         actor_idl, ap + "SetAppUpdateResponse",         false);

    // ── otaclient_service: registration + manager broadcasts (4 topics) ────
    const std::string cp = "/mmf/otamanager/otaclient_service/";
    ok &= add("SetOtaClientRegistration",         client_idl, cp + "SetOtaClientRegistration",         /*ros→raw*/true);
    ok &= add("SetOtaClientRegistrationResponse", client_idl, cp + "SetOtaClientRegistrationResponse", /*raw→ros*/false);
    ok &= add("NotifyOtaManagerReady",            client_idl, cp + "NotifyOtaManagerReady",            /*raw→ros*/false);
    ok &= add("NotifyAppUpdateStatus",            client_idl, cp + "NotifyAppUpdateStatus",            /*raw→ros*/false);

    if (!ok) {
        std::cerr << "[bridge] one or more pairs failed to register\n";
        return 1;
    }

    std::signal(SIGINT,  on_sigint);
    std::signal(SIGTERM, on_sigint);
    std::cout << "[bridge] running (idl_root=" << idl_root << "). Ctrl-C to exit.\n";
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "[bridge] exiting.\n";
    return 0;
}
