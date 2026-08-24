// ss2_control_bridge -- canonical SiLS VehicleControl (DDS) -> Autoware control topics.
// Loop B, work item B2-3.  Docs: adapters/ss2/README.md §9
//
// This is the RETURN half of the anti-corruption layer, and the piece that CLOSES THE LOOP.
// ss2_ego_bridge carries the simulator's state out to the vECU; this carries the vECU's
// command back in, so the next simulator frame is a consequence of what our ECU decided.
// Until this node runs, the ego sits still no matter what the vECU computes.
//
//   [vECU] ──VehicleControl──▶ [this node] ──/control/command/control_cmd──▶ concealer::
//            (DDS, canonical)                ──/control/command/gear_cmd───▶ AutowareUniverse
//                  ▲                                                             │
//                  └────────── EgoState ◀── ss2_ego_bridge ◀── /entity/status ◀──┘
//                                                                        vehicle model
//
// WHY IT IS A SEPARATE NODE FROM ss2_ego_bridge
//
// They fail independently and they are diagnosed differently.  A silent outbound path
// means the vECU sees nothing; a silent return path means the ego does not move while
// everything else looks healthy.  Keeping them apart means the report line of each says
// which one is dead, and lets stage 1 (open loop, B2-1) keep running unchanged -- the
// stage-1 verdict is a published artefact and must stay reproducible.
//
// NO AUTOWARE REQUIRED, and that is the point of B1-2's finding: concealer's data plane
// (AutowareUniverse) is a plain topic->struct adapter with no handshake.  Publishing these
// two topics moves the ego with ZERO upstream patch, whether the lifecycle side is our
// autoware_stub (stage 2) or a real Autoware (stage 3).  Only the remap changes.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <rclcpp/rclcpp.hpp>

#include <autoware_control_msgs/msg/control.hpp>
#include <autoware_vehicle_msgs/msg/gear_command.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Generated from idl/dds/sils/VehicleControl.idl by fastddsgen at build time.  Extension
// differs across generator majors (v4.x -> .hpp for Fast DDS 3.x, v3.x -> .h for 2.x);
// resolve at compile time, not at CMake configure time -- the files do not exist yet then.
#if __has_include("VehicleControlPubSubTypes.hpp")
#  include "VehicleControlPubSubTypes.hpp"
#else
#  include "VehicleControlPubSubTypes.h"
#endif

#include "ss2_control_mapping.hpp"
#include "aw_type_propagation.hpp"

namespace {

using namespace eprosima::fastdds::dds;
using sils::ss2::AutowareCommand;
using sils::ss2::AutowareGear;
using sils::ss2::CanonicalVehicleControl;

using AutowareControl = autoware_control_msgs::msg::Control;
using AutowareGearCommand = autoware_vehicle_msgs::msg::GearCommand;

// take_next_sample's OK constant is spelled differently across Fast DDS lines
// (2.x: ReturnCode_t::RETCODE_OK, 3.x: RETCODE_OK). The comparison MUST stay dependent on
// the template parameter -- a non-dependent qualified name is looked up when the template
// is DEFINED, so naming either spelling breaks the other line even though that branch is
// never taken. RETCODE_OK is the zero value on both, so a value-initialised code is the
// same thing and needs no name lookup at all. Same trick as ss2_ego_bridge.cpp's WriteOk.
template <typename ReturnCodeT>
inline bool TakeOk(const ReturnCodeT& r) {
    return r == ReturnCodeT{};
}

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

// Named -> generated constant.  Resolved BY NAME so a renumbered or renamed gear constant
// is a compile error rather than a car that silently drives backwards; see
// ss2_control_mapping.hpp's AutowareGear.  The default arm is unreachable for the enum as
// written, and exists so adding a value to AutowareGear without handling it here is caught
// by -Wswitch rather than by a scenario behaving oddly.
std::uint8_t ToGearConstant(AutowareGear g) {
    switch (g) {
        case AutowareGear::kNone:    return AutowareGearCommand::NONE;
        case AutowareGear::kNeutral: return AutowareGearCommand::NEUTRAL;
        case AutowareGear::kDrive:   return AutowareGearCommand::DRIVE;
        case AutowareGear::kReverse: return AutowareGearCommand::REVERSE;
        case AutowareGear::kPark:    return AutowareGearCommand::PARK;
    }
    return AutowareGearCommand::NONE;
}

// --- DDS side ----------------------------------------------------------------
//
// Reader for the vECU's `out` channel (config/sils/channelmap/ego-min.yaml:
// Vehicle.Control -> sils/ego/cmd, type canonical/vehicle-control).
//
// QoS: RELIABLE to match the vECU writer (vehicle_run.hpp apply_qos_w), and durability
// VOLATILE **deliberately**.
//
// This is the mirror of the rule the Autoware stub follows on the writing side.  A writer
// offers the STRONGEST it can, because an offer satisfies every weaker request; a reader
// asks for the WEAKEST it can live with, because a request is only satisfied by an equal
// or stronger offer.  Requesting TRANSIENT_LOCAL here would fail to match a vECU started
// with SILS_DURABILITY=volatile -- and an RxO mismatch is SILENT on both sides.  We do not
// want history anyway: a control command from ten seconds ago is not something to act on.
class VehicleControlReader : public DataReaderListener {
public:
    using Callback = std::function<void(const CanonicalVehicleControl&)>;

    VehicleControlReader(int domain_id, const std::string& topic_name, Callback on_sample)
        : on_sample_(std::move(on_sample)) {
        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            domain_id, PARTICIPANT_QOS_DEFAULT);
        if (participant_ == nullptr) {
            throw std::runtime_error("failed to create DomainParticipant on domain " +
                                     std::to_string(domain_id));
        }

        type_ = TypeSupport(new VehicleControlPubSubType());
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
        rqos.history().depth = 16;
        rqos.durability().kind = VOLATILE_DURABILITY_QOS;

        reader_ = subscriber_->create_datareader(topic_, rqos, this);
        if (reader_ == nullptr) throw std::runtime_error("failed to create DataReader");
    }

    ~VehicleControlReader() override {
        if (participant_ != nullptr) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    VehicleControlReader(const VehicleControlReader&) = delete;
    VehicleControlReader& operator=(const VehicleControlReader&) = delete;

    void on_data_available(DataReader* reader) override {
        VehicleControl msg;
        SampleInfo info;
        // Drain: the reader is called once per notification but several samples may have
        // arrived.  Only the newest matters for control, yet they are all taken so the
        // history does not fill and start rejecting the writer.
        while (TakeOk(reader->take_next_sample(&msg, &info))) {
            if (!info.valid_data) continue;
            CanonicalVehicleControl c;
            c.run_id = msg.run_id();
            c.seq = msg.seq();
            c.stamp_ns = msg.stamp_ns();
            c.steering_angle = msg.steering_angle();
            c.acceleration = msg.acceleration();
            c.gear = msg.gear();
            on_sample_(c);
        }
    }

    // Matched publisher count, for the "is anyone even talking to us" report.
    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& s) override {
        matched_.store(s.current_count);
    }
    int matched() const { return matched_.load(); }

private:
    Callback on_sample_;
    DomainParticipant* participant_ = nullptr;
    Subscriber* subscriber_ = nullptr;
    Topic* topic_ = nullptr;
    DataReader* reader_ = nullptr;
    TypeSupport type_;
    std::atomic<int> matched_{0};
};

// --- the node ----------------------------------------------------------------

class Ss2ControlBridge : public rclcpp::Node {
public:
    Ss2ControlBridge() : rclcpp::Node("ss2_control_bridge") {
        const std::string control_topic =
            declare_parameter<std::string>("control_topic", "/control/command/control_cmd");
        const std::string gear_topic =
            declare_parameter<std::string>("gear_topic", "/control/command/gear_cmd");
        // Every channel map in the tree publishes longitudinal actuation on sils/ego/cmd.
        // Override only for steer-min.yaml (sils/ego/steer_cmd), which is a different
        // actuator on purpose. A name mismatch here is SILENT -- the reader just never
        // matches -- so the startup line below prints the resolved name, and Report()
        // carries matched=0 for exactly this case.
        const std::string sils_topic =
            declare_parameter<std::string>("sils_topic", "sils/ego/cmd");
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        // 0 disables the watchdog. Default 1 s: the vECU ticks far faster than that (the
        // stage-1 runs were 100 Hz), so a full second of silence is ~100 missed ticks and
        // cannot be jitter.
        stale_after_ = std::chrono::duration<double>(
            declare_parameter<double>("stale_command_timeout_s", 1.0));

        // QoS(1) mirrors concealer's own subscription (autoware_universe.cpp). Control is
        // a stream of the latest value; queuing it would deliver stale commands late.
        pub_control_ = create_publisher<AutowareControl>(control_topic, rclcpp::QoS(1));
        pub_gear_ = create_publisher<AutowareGearCommand>(gear_topic, rclcpp::QoS(1));

        reader_ = std::make_unique<VehicleControlReader>(
            domain_id, sils_topic,
            [this](const CanonicalVehicleControl& c) { OnCommand(c); });

        watchdog_ = create_wall_timer(std::chrono::milliseconds(100), [this] { Watchdog(); });
        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "ss2_control_bridge: %s (DDS domain %d) -> %s + %s",
                    sils_topic.c_str(), domain_id, control_topic.c_str(), gear_topic.c_str());

        // Two things a reader of a surprising result needs to know up front, both of which
        // are consequences of the canonical contract rather than bugs.
        RCLCPP_WARN(get_logger(),
                    "longitudinal.velocity is published as 0.0 -- canonical VehicleControl "
                    "carries acceleration only (the IDL pins the longitudinal mode). That is "
                    "IGNORED by *_STEER_ACC* vehicle models, which the ego uses by default, "
                    "but a *_STEER_VEL model reads it as the commanded speed and the ego will "
                    "not move. If the car sits still with commands flowing, check "
                    "vehicle_model_type first.");
        if (stale_after_.count() > 0.0) {
            RCLCPP_INFO(get_logger(),
                        "watchdog: after %.1fs without a command, acceleration is withdrawn "
                        "(steering and gear held). concealer retains the LAST command forever, "
                        "so without this a crashed vECU leaves the ego driving itself -- and a "
                        "scenario it then completes would score a PASS nothing earned.",
                        stale_after_.count());
        } else {
            RCLCPP_WARN(get_logger(),
                        "watchdog DISABLED (stale_command_timeout_s=0). A vECU that stops "
                        "publishing will leave its last command applied indefinitely.");
        }
    }

private:
    void OnCommand(const CanonicalVehicleControl& c) {
        const AutowareCommand a = sils::ss2::map_control(c);

        if (!a.gear_recognized) {
            ++bad_gear_;
            // Rate-limited: a vECU sending a bad gear sends it every tick, and 100 Hz of
            // identical ERROR lines buries everything else in the log.
            RCLCPP_ERROR_THROTTLE(
                get_logger(), *get_clock(), 5000,
                "unrecognised canonical gear %u (expected 0=PARK 1=REVERSE 2=NEUTRAL "
                "3=DRIVE) -- commanding NONE, which zeroes the acceleration in the vehicle "
                "model. The ego will coast. seq=%llu",
                static_cast<unsigned>(c.gear), static_cast<unsigned long long>(c.seq));
        }

        {
            std::lock_guard<std::mutex> lock(mu_);
            last_ = a;
            last_seq_ = c.seq;
            have_last_ = true;
            last_arrival_ = now();
            withdrawn_ = false;
        }
        Publish(a);
        ++forwarded_;
    }

    // Publishes BOTH topics for every command.
    //
    // The gear is not "sticky" on the simulator side in the way it looks: concealer holds
    // the last GearCommand it received, so a gear published once would keep applying --
    // but only if it was ever received at all. Publishing both together means the pair is
    // always consistent, and a subscriber that joins late gets a full command on the next
    // tick rather than a steering angle with somebody else's gear.
    void Publish(const AutowareCommand& a) {
        const rclcpp::Time stamp = now();

        // Only the stamps and the three numbers concealer reads
        // (autoware_universe.cpp: lateral.steering_tire_angle, longitudinal.velocity,
        // longitudinal.acceleration). `control_time` and the is_defined_* flags come and
        // go across Autoware message generations, and filling a field that may not exist
        // buys nothing here while making the adapter refuse to build on half of them.
        AutowareControl control;
        control.stamp = stamp;
        control.lateral.stamp = stamp;
        control.lateral.steering_tire_angle = static_cast<float>(a.steering_tire_angle);
        control.longitudinal.stamp = stamp;
        control.longitudinal.velocity = static_cast<float>(a.longitudinal_velocity);
        control.longitudinal.acceleration = static_cast<float>(a.longitudinal_acceleration);
        pub_control_->publish(control);

        AutowareGearCommand gear;
        gear.stamp = stamp;
        gear.command = ToGearConstant(a.gear);
        pub_gear_->publish(gear);

        ++published_;
    }

    void Watchdog() {
        if (stale_after_.count() <= 0.0) return;

        AutowareCommand hold;
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (!have_last_) return;  // nothing to withdraw; we have never been driven
            if ((now() - last_arrival_).seconds() < stale_after_.count()) return;
            if (!withdrawn_) {
                withdrawn_ = true;
                RCLCPP_ERROR(get_logger(),
                             "no VehicleControl for %.1fs after seq=%llu -- withdrawing the "
                             "command (acceleration 0, steering and gear held). The ego will "
                             "coast. This is what a crashed or stalled vECU looks like; the "
                             "simulator would otherwise keep applying the last command.",
                             stale_after_.count(),
                             static_cast<unsigned long long>(last_seq_));
            }
            hold = sils::ss2::withdraw_command(last_);
            last_ = hold;
        }
        // Republished every tick while the silence lasts, deliberately: if the vECU comes
        // back, its next command simply overwrites this one, and if it does not, the zero
        // keeps being the freshest thing concealer has.
        Publish(hold);
        ++stale_holds_;
    }

    void Report() {
        const int matched = reader_->matched();
        if (forwarded_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no VehicleControl received yet (matched publishers=%d). "
                        "%s",
                        matched,
                        matched == 0
                            ? "Nothing is writing our topic: check VISS_DDS_DOMAIN_ID matches "
                              "the vECU's, that its channelmap has an `out` channel on this "
                              "topic, and that the vECU is actually running."
                            : "A writer is matched but has sent nothing -- the vECU is up but "
                              "not ticking, or its behavior produces no command.");
            return;
        }
        RCLCPP_INFO(get_logger(),
                    "forwarded=%llu published=%llu stale_holds=%llu bad_gear=%llu matched=%d",
                    static_cast<unsigned long long>(forwarded_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(stale_holds_),
                    static_cast<unsigned long long>(bad_gear_), matched);
    }

    rclcpp::Publisher<AutowareControl>::SharedPtr pub_control_;
    rclcpp::Publisher<AutowareGearCommand>::SharedPtr pub_gear_;
    rclcpp::TimerBase::SharedPtr watchdog_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::unique_ptr<VehicleControlReader> reader_;

    // The DDS listener runs on a Fast DDS thread while the timers run on the rclcpp
    // executor, so everything shared between them is behind this mutex.
    std::mutex mu_;
    AutowareCommand last_{};
    bool have_last_ = false;
    bool withdrawn_ = false;
    rclcpp::Time last_arrival_{0, 0, RCL_ROS_TIME};
    std::uint64_t last_seq_ = 0;

    std::chrono::duration<double> stale_after_{1.0};
    std::uint64_t forwarded_ = 0;
    std::uint64_t published_ = 0;
    std::uint64_t stale_holds_ = 0;
    std::uint64_t bad_gear_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<Ss2ControlBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("ss2_control_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
