#include "dds_bridge.h"

#include <cstdlib>
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
                                 std::string tag,
                                 DdsBridge* bridge)
    : writer_(writer), type_(type), tag_(std::move(tag)), bridge_(bridge) {}

void ForwardListener::on_data_available(DataReader* reader) {
    DynamicData::_ref_type data = DynamicDataFactory::get_instance()->create_data(type_);
    SampleInfo info;
    if (reader->take_next_sample(&data, &info) != RETCODE_OK) return;
    if (!info.valid_data) return;

    // Trust-on-first-use: cache the first observed publisher's prefix and
    // drop samples from any other participant. See header for the why.
    // GuidPrefix is the first 12 bytes of the 16-byte InstanceHandle_t value.
    int source_count = 0;
    {
        std::lock_guard<std::mutex> lk(trust_mutex_);
        const uint8_t* raw = reinterpret_cast<const uint8_t*>(&info.publication_handle);

        // Multi-source detection (independent of the trust pin): remember every
        // distinct publisher prefix. >1 ⇒ duplicate core/adsw on the domain.
        std::array<uint8_t, 12> prefix{};
        std::copy_n(raw, 12, prefix.begin());
        if (seen_prefixes_.insert(prefix).second && seen_prefixes_.size() > 1) {
            std::cout << "[bridge] " << tag_ << " ⚠ MULTIPLE publishers on domain (n="
                      << seen_prefixes_.size() << ") — duplicate core/adsw?\n";
            source_count = static_cast<int>(seen_prefixes_.size());
        }

        if (!trust_set_) {
            std::copy_n(raw, 12, trusted_prefix_.begin());
            trust_set_ = true;
            std::cout << "[bridge] " << tag_ << " trust pinned\n";
        } else if (!std::equal(trusted_prefix_.begin(), trusted_prefix_.end(), raw)) {
            std::cout << "[bridge] " << tag_ << " dropped (untrusted publisher)\n";
            // Still report the elevated source count (set updated above) before dropping.
            if (source_count > 1 && bridge_) bridge_->ReportSourceCount(tag_, source_count);
            return;
        }
    }
    if (source_count > 1 && bridge_) bridge_->ReportSourceCount(tag_, source_count);

    if (writer_->write(&data) == RETCODE_OK) {
        std::cout << "[bridge] " << tag_ << " forwarded\n";
    } else {
        std::cerr << "[bridge] " << tag_ << " write FAILED\n";
    }
}

void ForwardListener::on_subscription_matched(
    DataReader* /*reader*/, const SubscriptionMatchedStatus& info) {
    // Only react to a publisher leaving (a match removal).
    if (info.current_count_change >= 0) return;

    int source_count = -1;
    {
        std::lock_guard<std::mutex> lk(trust_mutex_);

        // GuidPrefix is the first 12 bytes of the 16-byte InstanceHandle_t value
        // — same slice used when the pin was set in on_data_available.
        const uint8_t* raw =
            reinterpret_cast<const uint8_t*>(&info.last_publication_handle);

        // Drop the departed publisher from the multi-source set so the count
        // (and any lfc warning) auto-clears when a duplicate goes away.
        std::array<uint8_t, 12> prefix{};
        std::copy_n(raw, 12, prefix.begin());
        if (seen_prefixes_.erase(prefix) > 0) {
            source_count = static_cast<int>(seen_prefixes_.size());
        }

        if (trust_set_ &&
            std::equal(trusted_prefix_.begin(), trusted_prefix_.end(), raw)) {
            trust_set_ = false;
            std::cout << "[bridge] " << tag_
                      << " trusted publisher left — re-pinning on next sample\n";
        }
    }
    // Report the new (lower) count so a stale ">1" warning clears in lfc.
    if (source_count >= 0 && bridge_) bridge_->ReportSourceCount(tag_, source_count);
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

    // This bridge spans the two domain planes.
    //
    //   DDS plane  (participant_)     : bare `lifecyclemgr::X` — MMF raw services.
    //                                   MMF_DDS_DOMAIN_ID > DDS_DOMAIN_ID > 0.
    //   ROS2 plane (ros_participant_) : ROS-canonical `…::msg::dds_::X_` — rosbridge,
    //                                   rviz2. ROS_DOMAIN_ID, defaulting to the DDS
    //                                   plane when unset.
    //
    // A ROS 2 topic on rmw_fastrtps is just a DDS topic with the `rt/` prefix and a
    // mangled type name, so the ROS side needs no rclcpp -- only a participant on the
    // right domain. That is why this stays a plain Fast-DDS program.
    //
    // Why two participants: a participant belongs to exactly ONE domain. Before this,
    // both sides shared one participant, so setting DDS_DOMAIN_ID=5 / ROS_DOMAIN_ID=42
    // put the forwarded `mmf/*` ROS-canonical topics on domain 5, where rosbridge (which
    // rmw puts on 42) could not see them -- the HMI silently lost all MMF data
    // (measured on board 172.16.10.151, 2026-09-04).
    //
    // When the two domains are EQUAL we deliberately keep using a single participant:
    // that is the common configuration and it keeps discovery traffic and behaviour
    // byte-identical to before this change.
    int mmf_domain = 0;
    if (const char* e = std::getenv("MMF_DDS_DOMAIN_ID")) mmf_domain = std::atoi(e);
    else if (const char* g = std::getenv("DDS_DOMAIN_ID")) mmf_domain = std::atoi(g);

    int ros_domain = mmf_domain;
    if (const char* r = std::getenv("ROS_DOMAIN_ID")) ros_domain = std::atoi(r);

    participant_ = factory->create_participant(mmf_domain, qos);
    if (!participant_) {
        std::cerr << "[bridge] create_participant failed (dds domain " << mmf_domain << ")\n";
        return false;
    }
    publisher_  = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!publisher_ || !subscriber_) {
        std::cerr << "[bridge] create publisher/subscriber failed\n";
        return false;
    }

    if (ros_domain == mmf_domain) {
        ros_participant_ = participant_;
        ros_publisher_   = publisher_;
        ros_subscriber_  = subscriber_;
        ros_plane_separate_ = false;
    } else {
        ros_participant_ = factory->create_participant(ros_domain, qos);
        if (!ros_participant_) {
            std::cerr << "[bridge] create_participant failed (ros domain " << ros_domain << ")\n";
            return false;
        }
        ros_publisher_  = ros_participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        ros_subscriber_ = ros_participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!ros_publisher_ || !ros_subscriber_) {
            std::cerr << "[bridge] create ros publisher/subscriber failed\n";
            return false;
        }
        ros_plane_separate_ = true;
    }

    std::cout << "[bridge] dds domain=" << mmf_domain
              << " ros domain=" << ros_domain
              << (ros_plane_separate_ ? " (two participants)" : " (single participant)")
              << "\n";
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
    // Each side is registered on ITS OWN plane's participant. When the two domains
    // are equal these are the same participant, which is the pre-existing behaviour.
    TypeSupport ts_raw(new DynamicPubSubType(dyn_type));
    ts_raw.register_type(participant_, cfg.raw_type);
    TypeSupport ts_ros(new DynamicPubSubType(dyn_type));
    ts_ros.register_type(ros_participant_, cfg.ros_type);

    Topic* raw_topic = participant_->create_topic(cfg.raw_topic, cfg.raw_type, TOPIC_QOS_DEFAULT);
    Topic* ros_topic = ros_participant_->create_topic(cfg.ros_topic, cfg.ros_type, TOPIC_QOS_DEFAULT);
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
    // Endpoint goes on the plane its topic belongs to — raw endpoints on the DDS
    // plane, ros endpoints on the ROS2 plane. Crossing the planes happens in the
    // listener callback (reader on one plane hands the sample to a writer on the
    // other), which is exactly what makes the two domains talk to each other.
    if (cfg.ros_to_raw) {
        writer = publisher_->create_datawriter(raw_topic, wqos);
        tag = cfg.struct_name + " (ros→raw)";
        auto listener = std::make_unique<ForwardListener>(writer, dyn_type, tag, this);
        reader = ros_subscriber_->create_datareader(ros_topic, rqos, listener.get());
        listeners_.push_back(std::move(listener));
    } else {
        writer = ros_publisher_->create_datawriter(ros_topic, wqos);
        tag = cfg.struct_name + " (raw→ros)";
        auto listener = std::make_unique<ForwardListener>(writer, dyn_type, tag, this);
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

bool DdsBridge::EnableSourceDiag(const std::string& ros_topic_no_slash) {
    // std_msgs/String — parsed from idl_root/std_msgs/String.idl, registered
    // under the ROS-canonical name so rosbridge resolves it as std_msgs/String.
    auto builder = DynamicTypeBuilderFactory::get_instance()
        ->create_type_w_uri(idl_root_ + "/std_msgs/String.idl", "String_", {idl_root_});
    if (!builder) {
        std::cerr << "[bridge] EnableSourceDiag: failed to parse std_msgs/String.idl\n";
        return false;
    }
    diag_type_ = builder->build();
    // ROS-canonical type on a ROS topic that lfc_monitor reads through rosbridge
    // -> this diagnostic belongs on the ROS2 plane, not the DDS plane.
    TypeSupport ts(new DynamicPubSubType(diag_type_));
    ts.register_type(ros_participant_, "std_msgs::msg::dds_::String_");

    const std::string ros_topic = "rt/" + ros_topic_no_slash;
    Topic* topic = ros_participant_->create_topic(ros_topic, "std_msgs::msg::dds_::String_",
                                                  TOPIC_QOS_DEFAULT);
    if (!topic) {
        std::cerr << "[bridge] EnableSourceDiag: create_topic failed for " << ros_topic << "\n";
        return false;
    }
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // Latch the latest warning so a late-joining lfc_monitor immediately sees
    // the current multi-source status (KEEP_LAST(1) bounds it to one sample).
    wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 1;
    diag_writer_ = ros_publisher_->create_datawriter(topic, wqos);
    if (!diag_writer_) {
        std::cerr << "[bridge] EnableSourceDiag: create_datawriter failed\n";
        return false;
    }
    std::cout << "[bridge] source-diag enabled on " << ros_topic << "\n";
    return true;
}

void DdsBridge::ReportSourceCount(const std::string& tag, int count) {
    if (!diag_writer_) return;
    std::lock_guard<std::mutex> lk(diag_mutex_);

    // Only emit on state change: enter warned (>1) or clear (<=1). Avoids
    // spamming lfc on every sample from the duplicate publisher.
    const bool warned = count > 1;
    const bool was_warned = diag_warned_tags_.count(tag) > 0;
    if (warned == was_warned) return;
    if (warned) diag_warned_tags_.insert(tag);
    else        diag_warned_tags_.erase(tag);

    // Minimal JSON (no external json dep in the bridge). tag is bridge-internal
    // (e.g. "LifecycleDiagReport (raw→ros)") — safe ASCII, no escaping needed.
    std::string json = "{\"topic\":\"" + tag + "\",\"publishers\":"
                     + std::to_string(count) + ",\"multi\":"
                     + (warned ? "true" : "false") + "}";

    DynamicData::_ref_type d = DynamicDataFactory::get_instance()->create_data(diag_type_);
    d->set_string_value(d->get_member_id_by_name("data"), json);
    if (diag_writer_->write(&d) == RETCODE_OK) {
        std::cout << "[bridge] source-diag: " << json << "\n";
    }
}
