#include "dds_bridge.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>

#include <iostream>
#include <utility>

using namespace eprosima::fastdds::dds;

// ── ForwardListener ────────────────────────────────────────────────────

ForwardListener::ForwardListener(DataWriter* writer,
                                 DynamicType::_ref_type type,
                                 std::string tag)
    : writer_(writer), type_(type), tag_(std::move(tag)) {}

void ForwardListener::on_data_available(DataReader* reader) {
    DynamicData::_ref_type data = DynamicDataFactory::get_instance()->create_data(type_);
    SampleInfo info;
    if (reader->take_next_sample(&data, &info) != RETCODE_OK) return;
    if (!info.valid_data) return;

    // Trust-on-first-use: cache the first observed publisher's prefix and
    // drop samples from any other participant. See header for the why.
    // GuidPrefix is the first 12 bytes of the 16-byte InstanceHandle_t value.
    {
        std::lock_guard<std::mutex> lk(trust_mutex_);
        const uint8_t* raw = reinterpret_cast<const uint8_t*>(&info.publication_handle);
        if (!trust_set_) {
            std::copy_n(raw, 12, trusted_prefix_.begin());
            trust_set_ = true;
            std::cout << "[bridge] " << tag_ << " trust pinned\n";
        } else if (!std::equal(trusted_prefix_.begin(), trusted_prefix_.end(), raw)) {
            std::cout << "[bridge] " << tag_ << " dropped (untrusted publisher)\n";
            return;
        }
    }

    if (writer_->write(&data) == RETCODE_OK) {
        std::cout << "[bridge] " << tag_ << " forwarded\n";
    } else {
        std::cerr << "[bridge] " << tag_ << " write FAILED\n";
    }
}

// ── DdsBridge ──────────────────────────────────────────────────────────

bool DdsBridge::Initialize(const std::string& idl_root) {
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

bool DdsBridge::AddPair(const PairConfig& cfg) {
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
    // Durable pairs (cfg.durable) keep the latest single sample so a bridge
    // that starts AFTER the producer still picks up a retained one-shot
    // (e.g. NotifyOtaManagerReady, published TRANSIENT_LOCAL at OtaManager
    // startup). KEEP_LAST(1) bounds it to one sample — no backlog.
    if (cfg.durable) {
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        rqos.history().kind = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth = 1;
    }

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // Default: force VOLATILE durability so the bridge doesn't replay
    // historical samples to late-subscribing peers. OtaManager publishes
    // responses with TRANSIENT_LOCAL, and without this override probe runs
    // would receive the cumulative backlog of previous response samples on
    // every reconnect.
    //
    // Durable pairs OVERRIDE this: republish TRANSIENT_LOCAL + KEEP_LAST(1)
    // so the retained one-shot survives to a late-joining ROS/rosbridge
    // subscriber. Safe only for single-sample latched topics (the depth-1
    // history means at most one sample is ever replayed) — never enable for
    // response streams.
    if (cfg.durable) {
        wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        wqos.history().kind = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth = 1;
    } else {
        wqos.durability().kind = VOLATILE_DURABILITY_QOS;
    }

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
