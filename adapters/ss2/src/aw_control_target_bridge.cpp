// aw_control_target_bridge -- REAL Autoware /control/command/* -> canonical ControlTarget.
// Stage 3, step 5.  Docs: docs/sils/sils-stage3-autoware-partition.md
//
// This is the piece that puts our vECU in Autoware's vehicle_interface seat:
//
//   [Autoware Control 파티션] ──/control/command/control_cmd──▶ [this node]
//                             ──/control/command/gear_cmd────▶      │
//                                                                   │ canonical ControlTarget
//                                                                   ▼
//                                                        autoware/control_target ──▶ vECU
//
// It is the mirror of ss2_control_bridge (which carries the vECU's command out to those
// same Autoware topics).  Same package, same Fast DDS/fastddsgen setup -- that setup is the
// fiddly part and duplicating it into a new package would buy nothing.  It is a SEPARATE
// PROCESS from the other two for the reason stated in ss2_control_bridge.cpp: they fail
// independently and a combined report line would not say which half is dead.
//
// ★ WHY THIS NODE SPEAKS FAST DDS ON BOTH SIDES
//
// The Autoware partitions run Cyclone (partition/run_partitions.sh sets
// RMW_IMPLEMENTATION=rmw_cyclonedds_cpp).  It is tempting to match them.  Do not:
//
//   - 2026-08-10 measurement: Fast DDS <-> Cyclone carries DISCOVERY and PUB/SUB fine, in
//     both directions, including this very type (autoware_control_msgs/msg/Control at
//     33 Hz).  Only SERVICE round-trips fail.  This node uses no services.
//   - The same day, switching our stack to Cyclone made the adapters die with
//     `ddsi_xt_type_add_typeobj with invalid type object` followed by SIGSEGV.
//
// So leaving RMW_IMPLEMENTATION at rmw_fastrtps_cpp keeps Cyclone out of our processes
// entirely while still reaching Autoware.  Evidence:
// docs/sils/evidence/adapi-real-partition-2026-08-10.md
//
// ★ TWO DOMAINS, ON PURPOSE
//
// ROS_DOMAIN_ID is Autoware's (43 in the measured bring-up).  VISS_DDS_DOMAIN_ID is the
// vECU's.  They are read separately and are expected to DIFFER -- that is what keeps the
// Autoware graph and the SiLS bus from discovering each other.
//
// ★ WHAT THIS DOES NOT DO
//
// It does not close the loop.  Nothing here feeds the vECU's output back to a plant; that
// is the next step and it needs the plant decision (Autoware's own
// simple_planning_simulator is currently consuming control_cmd itself).  Until then this is
// the stage-3 equivalent of stage 1: an OPEN loop that proves the wire.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <autoware_control_msgs/msg/control.hpp>
#include <autoware_vehicle_msgs/msg/gear_command.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Generated from idl/dds/sils/ControlTarget.idl by fastddsgen at build time.  Extension
// differs across generator majors -- see the same note in ss2_ego_bridge.cpp.
#if __has_include("ControlTargetPubSubTypes.hpp")
#  include "ControlTargetPubSubTypes.hpp"
#else
#  include "ControlTargetPubSubTypes.h"
#endif

#include "aw_target_mapping.hpp"
#include "aw_type_propagation.hpp"

namespace {

using namespace eprosima::fastdds::dds;
using sils::aw::AutowareControlSample;
using sils::aw::CanonicalControlTarget;
using sils::ss2::AutowareGear;

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

// write() returns bool on Fast DDS 2.x and ReturnCode_t on 3.x.  See the long explanation
// in ss2_ego_bridge.cpp -- the template comparison must stay dependent on its parameter.
inline bool WriteOk(bool r) { return r; }
template <typename ReturnCodeT>
inline bool WriteOk(const ReturnCodeT& r) { return r == ReturnCodeT{}; }

// GearCommand.command -> our ROS-free enum, BY NAME.
//
// The constants are static members of the generated message class, so a renamed or removed
// one is a compile error here rather than a wrong number on the wire.  That rule is
// inherited from ss2_control_mapping.hpp, where it is explained.
//
// DRIVE has numbered variants (DRIVE_2 .. DRIVE_18) and LOW/LOW_2 are also forward gears;
// they are all folded into kDrive.  REVERSE_2 folds into kReverse.  Anything left --
// including NONE -- falls through as kNone, which the mapping layer turns into a COASTING
// canonical gear and counts.  It must not become DRIVE.
AutowareGear MapGearCommand(std::uint8_t command) {
    using G = autoware_vehicle_msgs::msg::GearCommand;
    if (command == G::PARK) return AutowareGear::kPark;
    if (command == G::NEUTRAL) return AutowareGear::kNeutral;
    if (command == G::REVERSE || command == G::REVERSE_2) return AutowareGear::kReverse;
    if (command == G::LOW || command == G::LOW_2) return AutowareGear::kDrive;
    if (command >= G::DRIVE && command <= G::DRIVE_18) return AutowareGear::kDrive;
    return AutowareGear::kNone;
}

// --- DDS side ----------------------------------------------------------------
//
// QoS MUST match the vECU's ControlTarget reader (service/sils/vecu/src/vehicle_run.hpp):
// RELIABLE + KEEP_LAST(256), TRANSIENT_LOCAL unless SILS_DURABILITY=volatile.  It tracks
// the same env var for the same reason as every other writer in this tree -- a
// TRANSIENT_LOCAL reader against a VOLATILE writer does not match, and an RxO mismatch
// shows up as SILENCE, not as an error.
class ControlTargetWriter {
public:
    ControlTargetWriter(int domain_id, const std::string& topic_name, bool transient_local) {
        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos);
        if (participant_ == nullptr) {
            throw std::runtime_error("failed to create DomainParticipant on domain " +
                                     std::to_string(domain_id));
        }

        type_ = TypeSupport(new ControlTargetPubSubType());
        sils::ss2::DisableTypePropagation(type_);  // see aw_type_propagation.hpp
        type_.register_type(participant_);

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        topic_ = participant_->create_topic(topic_name, type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (publisher_ == nullptr || topic_ == nullptr) {
            throw std::runtime_error("failed to create publisher/topic for " + topic_name);
        }

        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        wqos.history().kind = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth = 256;
        if (transient_local) wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

        writer_ = publisher_->create_datawriter(topic_, wqos, nullptr);
        if (writer_ == nullptr) throw std::runtime_error("failed to create DataWriter");
    }

    ~ControlTargetWriter() {
        if (participant_ != nullptr) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    ControlTargetWriter(const ControlTargetWriter&) = delete;
    ControlTargetWriter& operator=(const ControlTargetWriter&) = delete;

    bool Write(const CanonicalControlTarget& t) {
        ControlTarget msg;
        msg.run_id(t.run_id);
        msg.seq(t.seq);
        msg.stamp_ns(t.stamp_ns);
        msg.steering_angle(t.steering_angle);
        msg.acceleration(t.acceleration);
        msg.gear(t.gear);
        return WriteOk(writer_->write(&msg));
    }

private:
    DomainParticipant* participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Topic* topic_ = nullptr;
    DataWriter* writer_ = nullptr;
    TypeSupport type_;
};

// --- the node ----------------------------------------------------------------

class AwControlTargetBridge : public rclcpp::Node {
public:
    AwControlTargetBridge() : rclcpp::Node("aw_control_target_bridge") {
        const std::string control_topic = declare_parameter<std::string>(
            "control_topic", "/control/command/control_cmd");
        const std::string gear_topic = declare_parameter<std::string>(
            "gear_topic", "/control/command/gear_cmd");
        const std::string target_topic = declare_parameter<std::string>(
            "target_topic", "autoware/control_target");
        run_id_ = declare_parameter<std::string>("run_id", GetEnvOr("SILS_RUN_ID", "aw-target"));
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        const bool transient_local = declare_parameter<bool>(
            "durability_transient_local",
            GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile");

        writer_ = std::make_unique<ControlTargetWriter>(domain_id, target_topic, transient_local);

        // Subscription QoS.  MEASURED on the real Control partition, 2026-08-10:
        //   /control/command/control_cmd  publisher = /control/vehicle_cmd_gate
        //   RELIABLE, KEEP_LAST(1), TRANSIENT_LOCAL
        // A VOLATILE request would still match (an offer may exceed a request), but asking
        // for TRANSIENT_LOCAL means a late-joining bridge gets the last command instead of
        // waiting up to a full period for the next one.  Depth 1 mirrors the offer: this is
        // a command stream where only the newest sample has any meaning, and a deeper queue
        // would let us fall behind and forward stale commands as if they were fresh.
        rclcpp::QoS cmd_qos(rclcpp::KeepLast(1));
        cmd_qos.reliable().transient_local();

        sub_control_ = create_subscription<autoware_control_msgs::msg::Control>(
            control_topic, cmd_qos,
            [this](autoware_control_msgs::msg::Control::ConstSharedPtr msg) { OnControl(*msg); });

        sub_gear_ = create_subscription<autoware_vehicle_msgs::msg::GearCommand>(
            gear_topic, cmd_qos,
            [this](autoware_vehicle_msgs::msg::GearCommand::ConstSharedPtr msg) {
                ++gear_received_;
                gear_ = MapGearCommand(msg->command);
            });

        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "aw_control_target_bridge: %s + %s (ROS 2 domain %s) -> %s "
                    "(DDS domain %d, %s), run_id=%s",
                    control_topic.c_str(), gear_topic.c_str(),
                    GetEnvOr("ROS_DOMAIN_ID", "0").c_str(), target_topic.c_str(), domain_id,
                    transient_local ? "TRANSIENT_LOCAL" : "VOLATILE", run_id_.c_str());

        // Say the dropped field out loud, once.  ControlTarget.idl carries no velocity, so
        // a reader of a trace must not assume the vECU ever saw Autoware's speed target.
        RCLCPP_WARN(get_logger(),
                    "DROPPED FIELD: Control.longitudinal.velocity is not carried -- "
                    "ControlTarget.idl pins the longitudinal mode to acceleration. A vECU "
                    "behaviour that wants to track a speed target cannot get it from this path.");
    }

private:
    void OnControl(const autoware_control_msgs::msg::Control& msg) {
        ++received_;

        AutowareControlSample s;
        s.stamp_ns = static_cast<std::int64_t>(msg.stamp.sec) * 1000000000LL +
                     static_cast<std::int64_t>(msg.stamp.nanosec);
        s.steering_tire_angle = msg.lateral.steering_tire_angle;
        s.longitudinal_acceleration = msg.longitudinal.acceleration;
        s.gear = gear_;

        const CanonicalControlTarget t =
            sils::aw::map_control_target(s, run_id_, seq_);
        if (!t.gear_recognized) ++bad_gear_;

        if (writer_->Write(t)) {
            ++published_;
            ++seq_;
        } else {
            ++write_failed_;
        }
    }

    void Report() {
        if (received_ == 0) {
            // The most common cause by far, and the one a bare "no samples" line would not
            // point at: Autoware publishes control_cmd only once it is ENGAGED.  A partition
            // sitting in STOP with no route emits nothing at all, which looks identical to a
            // domain or QoS problem from here.
            RCLCPP_WARN(get_logger(),
                        "no samples on %s yet. Check IN THIS ORDER: (1) Autoware is actually "
                        "ENGAGED -- /api/operation_mode/state mode==2; a partition in STOP "
                        "with no route publishes NOTHING; (2) ROS_DOMAIN_ID matches "
                        "Autoware's; (3) the publisher offers RELIABLE+TRANSIENT_LOCAL as "
                        "measured -- a changed offer would silently fail to match.",
                        sub_control_->get_topic_name());
            return;
        }
        RCLCPP_INFO(get_logger(),
                    "received=%llu published=%llu gear_msgs=%llu bad_gear=%llu write_failed=%llu",
                    static_cast<unsigned long long>(received_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(gear_received_),
                    static_cast<unsigned long long>(bad_gear_),
                    static_cast<unsigned long long>(write_failed_));
        if (gear_received_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no GearCommand seen -- every target so far went out as a COASTING "
                        "canonical gear (bad_gear=%llu). The vehicle will not accelerate.",
                        static_cast<unsigned long long>(bad_gear_));
        }
    }

    rclcpp::Subscription<autoware_control_msgs::msg::Control>::SharedPtr sub_control_;
    rclcpp::Subscription<autoware_vehicle_msgs::msg::GearCommand>::SharedPtr sub_gear_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::unique_ptr<ControlTargetWriter> writer_;

    std::string run_id_;
    AutowareGear gear_ = AutowareGear::kNone;

    std::uint64_t seq_ = 0;
    std::uint64_t received_ = 0;
    std::uint64_t published_ = 0;
    std::uint64_t gear_received_ = 0;
    std::uint64_t bad_gear_ = 0;
    std::uint64_t write_failed_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<AwControlTargetBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("aw_control_target_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
