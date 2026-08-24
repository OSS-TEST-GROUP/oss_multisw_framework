// aw_vehicle_status_bridge -- canonical SiLS EgoState -> Autoware /vehicle/status/*.
// Stage 3, step 5.  Docs: docs/sils/sils-integration-topology.md §3
//
// THE RETURN HALF of the vehicle_interface seat, and the piece both real configurations
// need:
//
//   플랜트(MORAI / Web.Auto) ──EgoState──▶ vECU ──VehicleControl──▶ 플랜트
//                                           │  ▲
//                            ControlTarget  │  └── [this node] ──▶ Autoware
//                    (aw_control_target_bridge)     /vehicle/status/*
//
// ★ WHY IT IS NOT OPTIONAL
//
// In the real configurations Autoware's own autoware_simple_planning_simulator is OFF --
// the plant is MORAI or SS_v2. That simulator is currently the ONLY publisher of
// /vehicle/status/*, and Autoware subscribes to it from all over: measured 2026-08-10 on a
// live Control partition,
//
//   velocity_status 3 subs · steering_status 7 · gear_status 4 · control_mode 5
//   turn_indicators_status 3 · hazard_lights_status 3        (25 total)
//
// Turn the simulator off without supplying these and 25 subscriptions starve. The symptom
// is visible in the partition log before localization: "[AEB] waiting for ego velocity",
// "waiting for current_twist msg", "Control is skipped since input data is not ready".
//
// ★ CONTROL MODE IS THE ONE THAT IS NOT A MEASUREMENT
//
// Five of the six topics are pure state translation. /vehicle/status/control_mode is
// different: it is the vehicle interface ASSERTING whether the vehicle is under autonomous
// control, and Autoware gates on it. We publish AUTONOMOUS because in this rig the vECU
// exists precisely to execute Autoware's commands -- but that is a CLAIM about the rig, not
// a reading from the plant, and EgoState carries nothing that could support it. It is a
// parameter (control_mode) so a configuration that means something else can say so, and it
// is called out here so nobody reads it as measured.
//
// ★ QoS MEASURED 2026-08-10: all six are RELIABLE, KEEP_LAST(1), VOLATILE. Publish with
//   exactly that -- a mismatched offer would leave those 25 subscriptions silently unmatched,
//   which is the failure this stack keeps paying for.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <autoware_vehicle_msgs/msg/control_mode_report.hpp>
#include <autoware_vehicle_msgs/msg/gear_report.hpp>
#include <autoware_vehicle_msgs/msg/hazard_lights_report.hpp>
#include <autoware_vehicle_msgs/msg/steering_report.hpp>
#include <autoware_vehicle_msgs/msg/turn_indicators_report.hpp>
#include <autoware_vehicle_msgs/msg/velocity_report.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#if __has_include("EgoStatePubSubTypes.hpp")
#  include "EgoStatePubSubTypes.hpp"
#else
#  include "EgoStatePubSubTypes.h"
#endif

#include "aw_stamp_policy.hpp"
#include "aw_type_propagation.hpp"
#include "ss2_control_mapping.hpp"  // CanonicalGear

namespace {

using namespace eprosima::fastdds::dds;
using sils::ss2::CanonicalGear;

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

// take_next_sample's OK constant is spelled differently across Fast DDS lines
// (2.x: ReturnCode_t::RETCODE_OK, 3.x: RETCODE_OK). The comparison MUST stay dependent on
// the template parameter -- naming either spelling directly breaks the other line even
// though that branch is never taken. Identical to ss2_control_bridge.cpp's TakeOk, which
// is where the full explanation lives.
template <typename ReturnCodeT>
inline bool TakeOk(const ReturnCodeT& r) {
    return r == ReturnCodeT{};
}

// DDS-free snapshot handed from the reader thread to the node.
struct EgoSnapshot {
    std::int64_t stamp_ns = 0;
    double vx = 0.0, vy = 0.0;
    double yaw_rate = 0.0;
    double steering_angle = 0.0;
    std::uint8_t gear = static_cast<std::uint8_t>(CanonicalGear::kNeutral);
};

// --- DDS side ----------------------------------------------------------------
//
// QoS mirrors the writer this consumes (ss2_ego_bridge / aw_ego_state_bridge / a MORAI
// adapter): RELIABLE + KEEP_LAST(256), TRANSIENT_LOCAL unless SILS_DURABILITY=volatile.
// Tracking that env var is mandatory -- a TRANSIENT_LOCAL reader against a VOLATILE writer
// does not match, silently.
class EgoStateReader : public DataReaderListener {
public:
    using Callback = std::function<void(const EgoSnapshot&)>;

    EgoStateReader(int domain_id, const std::string& topic_name, bool transient_local,
                   Callback cb)
        : cb_(std::move(cb)) {
        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos);
        if (participant_ == nullptr) {
            throw std::runtime_error("failed to create DomainParticipant on domain " +
                                     std::to_string(domain_id));
        }
        type_ = TypeSupport(new EgoStatePubSubType());
        sils::ss2::DisableTypePropagation(type_);  // see aw_type_propagation.hpp
        type_.register_type(participant_);

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        topic_ = participant_->create_topic(topic_name, type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (subscriber_ == nullptr || topic_ == nullptr) {
            throw std::runtime_error("failed to create subscriber/topic for " + topic_name);
        }

        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.history().kind = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth = 256;
        if (transient_local) rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

        reader_ = subscriber_->create_datareader(topic_, rqos, this);
        if (reader_ == nullptr) throw std::runtime_error("failed to create DataReader");
    }

    ~EgoStateReader() override {
        if (participant_ != nullptr) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    EgoStateReader(const EgoStateReader&) = delete;
    EgoStateReader& operator=(const EgoStateReader&) = delete;

    void on_data_available(DataReader* reader) override {
        EgoState s;
        SampleInfo info;
        // Drain, do not take one. The producer runs faster than nothing in particular and a
        // one-per-callback loop would fall behind on a burst; only the newest sample means
        // anything for a status report, so take them all and let the last one win.
        while (TakeOk(reader->take_next_sample(&s, &info))) {
            if (!info.valid_data) continue;
            EgoSnapshot snap;
            snap.stamp_ns = s.stamp_ns();
            snap.vx = s.velocity().x();
            snap.vy = s.velocity().y();
            snap.yaw_rate = s.yaw_rate();
            snap.steering_angle = s.steering_angle();
            snap.gear = s.gear();
            cb_(snap);
        }
    }

private:
    DomainParticipant* participant_ = nullptr;
    Subscriber* subscriber_ = nullptr;
    Topic* topic_ = nullptr;
    DataReader* reader_ = nullptr;
    TypeSupport type_;
    Callback cb_;
};

// --- the node ----------------------------------------------------------------

class AwVehicleStatusBridge : public rclcpp::Node {
public:
    AwVehicleStatusBridge() : rclcpp::Node("aw_vehicle_status_bridge") {
        const std::string sils_topic = declare_parameter<std::string>(
            "sils_topic", "sils/ego/state");
        const std::string prefix = declare_parameter<std::string>(
            "status_prefix", "/vehicle/status");
        frame_id_ = declare_parameter<std::string>("frame_id", "base_link");
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        const bool transient_local = declare_parameter<bool>(
            "durability_transient_local",
            GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile");
        // See the CONTROL MODE note at the top: a claim about the rig, not a measurement.
        control_mode_ = static_cast<std::uint8_t>(declare_parameter<int>(
            "control_mode", autoware_vehicle_msgs::msg::ControlModeReport::AUTONOMOUS));

        // DEFAULT epoch-anchored, unlike a pure relay. Everything downstream of this node
        // is Autoware, which ages messages against its own wall clock; a plant that stamps
        // on simulation time (SS_v2 does -- stamps start at 0) makes every report look 56
        // years stale and topic_state_monitor blocks autonomous mode. Set stamp_policy to
        // "source" only when the plant already stamps on the wall clock.
        // See aw_stamp_policy.hpp and docs/sils/evidence/ss2-clock-survey-2026-08-11.md.
        const std::string policy_text = declare_parameter<std::string>(
            "stamp_policy", GetEnvOr("SILS_STAMP_POLICY", "epoch-anchored"));
        if (!sils::ss2::parse_stamp_policy(policy_text, stamp_policy_)) {
            throw std::runtime_error(
                "unknown stamp_policy '" + policy_text +
                "'. Expected \"source\" or \"epoch-anchored\". Refusing to guess: picking "
                "the wrong one of these is silent -- the stamps stay plausible and only "
                "Autoware's freshness check notices.");
        }
        anchor_ = sils::ss2::StampAnchor(static_cast<std::int64_t>(
            declare_parameter<int>("stamp_anchor_tolerance_ms",
                                   sils::ss2::kDefaultAnchorToleranceNs / 1'000'000)) * 1'000'000);

        // MEASURED: every /vehicle/status/* publisher on the live Control partition offers
        // RELIABLE, KEEP_LAST(1), VOLATILE. Match it exactly.
        rclcpp::QoS qos(rclcpp::KeepLast(1));
        qos.reliable().durability_volatile();

        pub_velocity_ = create_publisher<autoware_vehicle_msgs::msg::VelocityReport>(
            prefix + "/velocity_status", qos);
        pub_steering_ = create_publisher<autoware_vehicle_msgs::msg::SteeringReport>(
            prefix + "/steering_status", qos);
        pub_gear_ = create_publisher<autoware_vehicle_msgs::msg::GearReport>(
            prefix + "/gear_status", qos);
        pub_mode_ = create_publisher<autoware_vehicle_msgs::msg::ControlModeReport>(
            prefix + "/control_mode", qos);
        pub_turn_ = create_publisher<autoware_vehicle_msgs::msg::TurnIndicatorsReport>(
            prefix + "/turn_indicators_status", qos);
        pub_hazard_ = create_publisher<autoware_vehicle_msgs::msg::HazardLightsReport>(
            prefix + "/hazard_lights_status", qos);

        reader_ = std::make_unique<EgoStateReader>(
            domain_id, sils_topic, transient_local,
            [this](const EgoSnapshot& s) { OnEgoState(s); });

        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "aw_vehicle_status_bridge: %s (DDS domain %d, %s) -> %s/* "
                    "(ROS 2 domain %s), frame_id=%s, control_mode=%u",
                    sils_topic.c_str(), domain_id,
                    transient_local ? "TRANSIENT_LOCAL" : "VOLATILE", prefix.c_str(),
                    GetEnvOr("ROS_DOMAIN_ID", "0").c_str(), frame_id_.c_str(),
                    static_cast<unsigned>(control_mode_));

        RCLCPP_INFO(get_logger(), "stamp_policy=%s (tolerance %lld ms)",
                    sils::ss2::stamp_policy_name(stamp_policy_),
                    static_cast<long long>(anchor_.tolerance_ns() / 1'000'000));

        RCLCPP_WARN(get_logger(),
                    "ASSERTED, NOT MEASURED: control_mode=%u is a claim about this rig -- "
                    "EgoState carries nothing that could support it. Turn indicators and "
                    "hazard lights are published as OFF for the same reason: the canonical "
                    "contract does not carry them.",
                    static_cast<unsigned>(control_mode_));
    }

private:
    // Autoware's GearReport constants, resolved BY NAME (see ss2_control_mapping.hpp for
    // why never by number). An unrecognised canonical octet becomes NONE rather than DRIVE:
    // reporting a forward gear the plant never claimed would let Autoware plan motion on a
    // direction we invented.
    std::uint8_t MapGear(std::uint8_t canonical) const {
        using G = autoware_vehicle_msgs::msg::GearReport;
        switch (static_cast<CanonicalGear>(canonical)) {
            case CanonicalGear::kPark:    return G::PARK;
            case CanonicalGear::kReverse: return G::REVERSE;
            case CanonicalGear::kNeutral: return G::NEUTRAL;
            case CanonicalGear::kDrive:   return G::DRIVE;
        }
        return G::NONE;
    }

    void OnEgoState(const EgoSnapshot& s) {
        ++received_;

        // Use the SOURCE's timestamp, not now(). Autoware's consumers time-align these
        // against each other and against its own clock; stamping with arrival time would
        // hide plant latency and desynchronise the reports from the state they describe.
        //
        // Under epoch-anchored the source stamp is SHIFTED by a constant, never re-timed:
        // the spacing between samples is still the plant's, so latency and jitter survive.
        // What changes is only the origin. StampAnchor also watches every later sample for
        // the anchor going stale (aw_stamp_policy.hpp).
        std::int64_t stamp_ns = static_cast<std::int64_t>(s.stamp_ns);
        if (stamp_policy_ == sils::ss2::StampPolicy::kEpochAnchored) {
            stamp_ns = anchor_.Apply(stamp_ns, now().nanoseconds());
            // Warm-up: the anchor is still settling and these samples are the
            // TRANSIENT_LOCAL backlog, i.e. the plant's history rather than its state.
            // Publishing them would walk Autoware's ego through several seconds of its own
            // past at burst rate. Drop and count.
            if (!anchor_.latched()) {
                ++warmup_dropped_;
                return;
            }
        }
        rclcpp::Time stamp(stamp_ns);

        autoware_vehicle_msgs::msg::VelocityReport v;
        v.header.stamp = stamp;
        v.header.frame_id = frame_id_;
        v.longitudinal_velocity = static_cast<float>(s.vx);
        v.lateral_velocity = static_cast<float>(s.vy);
        v.heading_rate = static_cast<float>(s.yaw_rate);
        pub_velocity_->publish(v);

        autoware_vehicle_msgs::msg::SteeringReport st;
        st.stamp = stamp;
        st.steering_tire_angle = static_cast<float>(s.steering_angle);  // both rad: 1:1
        pub_steering_->publish(st);

        autoware_vehicle_msgs::msg::GearReport g;
        g.stamp = stamp;
        g.report = MapGear(s.gear);
        if (g.report == autoware_vehicle_msgs::msg::GearReport::NONE) ++bad_gear_;
        pub_gear_->publish(g);

        autoware_vehicle_msgs::msg::ControlModeReport m;
        m.stamp = stamp;
        m.mode = control_mode_;
        pub_mode_->publish(m);

        // The canonical contract carries neither indicator; publishing DISABLE/OFF is the
        // honest "not signalling", and the startup banner says it is not measured.
        autoware_vehicle_msgs::msg::TurnIndicatorsReport ti;
        ti.stamp = stamp;
        ti.report = autoware_vehicle_msgs::msg::TurnIndicatorsReport::DISABLE;
        pub_turn_->publish(ti);

        autoware_vehicle_msgs::msg::HazardLightsReport hz;
        hz.stamp = stamp;
        hz.report = autoware_vehicle_msgs::msg::HazardLightsReport::DISABLE;
        pub_hazard_->publish(hz);

        ++published_;
    }

    void Report() {
        if (received_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no EgoState received yet. Check IN THIS ORDER: (1) something is "
                        "publishing sils/ego/state -- a plant adapter, or the stand-in "
                        "aw_ego_state_bridge; (2) VISS_DDS_DOMAIN_ID matches that writer's; "
                        "(3) SILS_DURABILITY matches -- a TRANSIENT_LOCAL reader against a "
                        "VOLATILE writer never matches, and says nothing about it.");
            return;
        }
        RCLCPP_INFO(get_logger(), "received=%llu published=%llu bad_gear=%llu "
                    "warmup_dropped=%llu",
                    static_cast<unsigned long long>(received_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(bad_gear_),
                    static_cast<unsigned long long>(warmup_dropped_));
        ReportAnchor();
    }

    // Split out because both this node and aw_localization_bridge need the identical
    // wording, and because the anchor's health is a DIFFERENT question from the counters
    // above: those can all look perfect while the stamps quietly age out of Autoware's
    // freshness window.
    void ReportAnchor() {
        if (stamp_policy_ != sils::ss2::StampPolicy::kEpochAnchored) return;

        // A warm-up closes in 500 ms and this report runs every 5 s, so seeing it still
        // open is never normal -- and while it is open NOTHING is published. Say so here,
        // because the counter line alone (received rising, published frozen) has already
        // been misread as healthy once.
        if (!anchor_.latched()) {
            RCLCPP_WARN(get_logger(),
                        "STAMP ANCHOR NOT LATCHED after %llu warm-up samples "
                        "(run_boundaries=%llu). NOTHING IS BEING PUBLISHED. The warm-up "
                        "reopens whenever the source clock jumps backwards; one restart is "
                        "normal, a warm-up that never closes means the clock keeps going "
                        "backwards -- almost always TWO WRITERS on the canonical topic "
                        "rather than one plant. Look for a leftover plant process before "
                        "suspecting this bridge.",
                        static_cast<unsigned long long>(anchor_.warmup_samples()),
                        static_cast<unsigned long long>(anchor_.run_boundaries()));
            return;
        }
        if (anchor_.exceedances() == 0) {
            RCLCPP_INFO(get_logger(),
                        "stamp anchor ok: max deviation %.1f ms (tolerance %lld), "
                        "plant restarts ridden through: %llu",
                        anchor_.max_deviation_ns() / 1e6,
                        static_cast<long long>(anchor_.tolerance_ns() / 1'000'000),
                        static_cast<unsigned long long>(anchor_.run_boundaries()));
            return;
        }
        RCLCPP_WARN(get_logger(),
                    "STAMP ANCHOR STALE: %llu samples past tolerance, max deviation %.1f ms. "
                    "Simulation time is no longer advancing at wall rate, so the offset "
                    "latched at the first sample no longer lands these reports in Autoware's "
                    "freshness window -- and nothing else will say so. Check the plant's "
                    "realtime_factor (must be 1.0) and use_sim_time. If the rig genuinely "
                    "needs a non-unit factor, this policy is the wrong one: publish the "
                    "plant's clock onto Autoware's domain and run it with use_sim_time.",
                    static_cast<unsigned long long>(anchor_.exceedances()),
                    anchor_.max_deviation_ns() / 1e6);
    }

    rclcpp::Publisher<autoware_vehicle_msgs::msg::VelocityReport>::SharedPtr pub_velocity_;
    rclcpp::Publisher<autoware_vehicle_msgs::msg::SteeringReport>::SharedPtr pub_steering_;
    rclcpp::Publisher<autoware_vehicle_msgs::msg::GearReport>::SharedPtr pub_gear_;
    rclcpp::Publisher<autoware_vehicle_msgs::msg::ControlModeReport>::SharedPtr pub_mode_;
    rclcpp::Publisher<autoware_vehicle_msgs::msg::TurnIndicatorsReport>::SharedPtr pub_turn_;
    rclcpp::Publisher<autoware_vehicle_msgs::msg::HazardLightsReport>::SharedPtr pub_hazard_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::unique_ptr<EgoStateReader> reader_;

    std::string frame_id_;
    std::uint8_t control_mode_ = 1;
    sils::ss2::StampPolicy stamp_policy_ = sils::ss2::StampPolicy::kEpochAnchored;
    sils::ss2::StampAnchor anchor_{};
    std::uint64_t warmup_dropped_ = 0;
    std::uint64_t received_ = 0;
    std::uint64_t published_ = 0;
    std::uint64_t bad_gear_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<AwVehicleStatusBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("aw_vehicle_status_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
