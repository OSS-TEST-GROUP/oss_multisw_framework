// aw_ego_state_bridge -- REAL Autoware ADAPI vehicle state -> canonical SiLS EgoState.
// Stage 3, step 5.  Docs: docs/sils/sils-stage3-autoware-partition.md
//
//   [Autoware] ──/api/vehicle/kinematics──▶ [this node] ──sils/ego/state──▶ vECU
//              ──/api/vehicle/status─────▶
//
// Sibling of aw_control_target_bridge (which carries Autoware's COMMAND in). Together they
// let a developer ECU run against a real Autoware without SS_v2, without concealer, and
// without unifying the RMW -- both are pure pub/sub, and 2026-08-10 measurement showed
// pub/sub crosses Fast DDS <-> Cyclone in both directions while SERVICES do not.
//
// ★ QoS IS MEASURED, NOT ASSUMED
//
// Both ADAPI topics are offered BEST_EFFORT by the Control partition (measured
// 2026-08-10). A RELIABLE reader NEVER matches a BEST_EFFORT writer and DDS declines that
// pairing in silence -- exactly the bug found the same day in
// service/core/vehicle_info_mgr's DDSProvider, where /api/vehicle/status was requested
// RELIABLE and therefore delivered nothing while the publisher ran at 10 Hz.
//
// ★ THIS IS A STAND-IN. See the long note in aw_state_mapping.hpp: the ego state here is
//   Autoware's own simple_planning_simulator reporting on itself, so an ECU fed from this
//   bridge CANNOT influence what it reads. Useful for exercising an ECU against real
//   Autoware; not a closed loop, and not the target wiring.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <autoware_adapi_v1_msgs/msg/vehicle_kinematics.hpp>
#include <autoware_adapi_v1_msgs/msg/vehicle_status.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#if __has_include("EgoStatePubSubTypes.hpp")
#  include "EgoStatePubSubTypes.hpp"
#else
#  include "EgoStatePubSubTypes.h"
#endif

#include "aw_state_mapping.hpp"
#include "aw_type_propagation.hpp"

namespace {

using namespace eprosima::fastdds::dds;
using sils::aw::AdapiGear;
using sils::aw::AutowareStateSample;
using sils::aw::CanonicalEgoState;

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

// bool on Fast DDS 2.x, ReturnCode_t on 3.x -- see ss2_ego_bridge.cpp for why the template
// comparison must stay dependent on its parameter.
inline bool WriteOk(bool r) { return r; }
template <typename ReturnCodeT>
inline bool WriteOk(const ReturnCodeT& r) { return r == ReturnCodeT{}; }

AdapiGear MapAdapiGear(std::uint8_t status) {
    using G = autoware_adapi_v1_msgs::msg::Gear;
    if (status == G::PARK) return AdapiGear::kPark;
    if (status == G::REVERSE) return AdapiGear::kReverse;
    if (status == G::NEUTRAL) return AdapiGear::kNeutral;
    if (status == G::DRIVE) return AdapiGear::kDrive;
    if (status == G::LOW) return AdapiGear::kLow;
    return AdapiGear::kUnknown;
}

EgoStateVec3 MakeVec3(const sils::aw::Vec3& v) {
    EgoStateVec3 out;
    out.x(v.x);
    out.y(v.y);
    out.z(v.z);
    return out;
}

// QoS mirrors the vECU's EgoState reader (service/sils/vecu/src/vehicle_run.hpp) and the
// existing ss2_ego_bridge writer it stands in for: RELIABLE + KEEP_LAST(256),
// TRANSIENT_LOCAL unless SILS_DURABILITY=volatile. Tracking that env var with the vECU is
// mandatory -- the mismatch does not error, it goes quiet.
class EgoStateWriter {
public:
    EgoStateWriter(int domain_id, const std::string& topic_name, bool transient_local) {
        DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id, pqos);
        if (participant_ == nullptr) {
            throw std::runtime_error("failed to create DomainParticipant on domain " +
                                     std::to_string(domain_id));
        }
        type_ = TypeSupport(new EgoStatePubSubType());
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

    ~EgoStateWriter() {
        if (participant_ != nullptr) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    EgoStateWriter(const EgoStateWriter&) = delete;
    EgoStateWriter& operator=(const EgoStateWriter&) = delete;

    bool Write(const CanonicalEgoState& s) {
        EgoState msg;
        msg.run_id(s.run_id);
        msg.seq(s.seq);
        msg.stamp_ns(s.stamp_ns);
        msg.position(MakeVec3(s.position));
        msg.orientation_rpy(MakeVec3(s.orientation_rpy));
        msg.velocity(MakeVec3(s.velocity));
        msg.acceleration(MakeVec3(s.acceleration));
        msg.yaw_rate(s.yaw_rate);
        msg.steering_angle(s.steering_angle);
        msg.gear(s.gear);
        return WriteOk(writer_->write(&msg));
    }

private:
    DomainParticipant* participant_ = nullptr;
    Publisher* publisher_ = nullptr;
    Topic* topic_ = nullptr;
    DataWriter* writer_ = nullptr;
    TypeSupport type_;
};

class AwEgoStateBridge : public rclcpp::Node {
public:
    AwEgoStateBridge() : rclcpp::Node("aw_ego_state_bridge") {
        const std::string kin_topic = declare_parameter<std::string>(
            "kinematics_topic", "/api/vehicle/kinematics");
        const std::string status_topic = declare_parameter<std::string>(
            "status_topic", "/api/vehicle/status");
        const std::string sils_topic = declare_parameter<std::string>(
            "sils_topic", "sils/ego/state");
        run_id_ = declare_parameter<std::string>("run_id", GetEnvOr("SILS_RUN_ID", "aw-state"));
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        const bool transient_local = declare_parameter<bool>(
            "durability_transient_local",
            GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile");

        writer_ = std::make_unique<EgoStateWriter>(domain_id, sils_topic, transient_local);

        // MEASURED: both ADAPI topics are offered BEST_EFFORT, KEEP_LAST(1), VOLATILE.
        // Request BEST_EFFORT so the reader matches; a RELIABLE request would go silent.
        rclcpp::QoS qos(rclcpp::KeepLast(1));
        qos.best_effort().durability_volatile();

        sub_kin_ = create_subscription<autoware_adapi_v1_msgs::msg::VehicleKinematics>(
            kin_topic, qos,
            [this](autoware_adapi_v1_msgs::msg::VehicleKinematics::ConstSharedPtr m) {
                OnKinematics(*m);
            });

        sub_status_ = create_subscription<autoware_adapi_v1_msgs::msg::VehicleStatus>(
            status_topic, qos,
            [this](autoware_adapi_v1_msgs::msg::VehicleStatus::ConstSharedPtr m) {
                ++status_received_;
                sample_.steering_tire_angle = m->steering_tire_angle;
                sample_.gear = MapAdapiGear(m->gear.status);
                sample_.have_status = true;
            });

        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "aw_ego_state_bridge: %s + %s (ROS 2 domain %s) -> %s "
                    "(DDS domain %d, %s), run_id=%s",
                    kin_topic.c_str(), status_topic.c_str(),
                    GetEnvOr("ROS_DOMAIN_ID", "0").c_str(), sils_topic.c_str(), domain_id,
                    transient_local ? "TRANSIENT_LOCAL" : "VOLATILE", run_id_.c_str());

        RCLCPP_WARN(get_logger(),
                    "STAND-IN SOURCE: this state is Autoware's own simulator reporting on "
                    "itself, not a plant the vECU can influence. A run using this bridge is "
                    "OPEN LOOP no matter what the ECU computes -- do not report it as closed.");
    }

private:
    // The kinematics message is the one with a clock, so it drives publication: one EgoState
    // per kinematics sample, carrying whatever the latest status said. Publishing on status
    // instead would emit duplicate kinematics, and publishing on both would double the rate
    // with half the samples stale.
    void OnKinematics(const autoware_adapi_v1_msgs::msg::VehicleKinematics& m) {
        ++kin_received_;

        const auto& p = m.pose.pose.pose;
        const auto& t = m.twist.twist.twist;
        const auto& a = m.accel.accel.accel;

        sample_.stamp_ns = static_cast<std::int64_t>(m.pose.header.stamp.sec) * 1000000000LL +
                           static_cast<std::int64_t>(m.pose.header.stamp.nanosec);
        sample_.position = {p.position.x, p.position.y, p.position.z};
        sample_.qx = p.orientation.x;
        sample_.qy = p.orientation.y;
        sample_.qz = p.orientation.z;
        sample_.qw = p.orientation.w;
        sample_.twist_linear = {t.linear.x, t.linear.y, t.linear.z};
        sample_.twist_angular = {t.angular.x, t.angular.y, t.angular.z};
        sample_.accel_linear = {a.linear.x, a.linear.y, a.linear.z};

        const CanonicalEgoState s = sils::aw::map_ego_state(sample_, run_id_, seq_);
        if (!s.gear_known) ++gear_unknown_;

        if (writer_->Write(s)) {
            ++published_;
            ++seq_;
        } else {
            ++write_failed_;
        }
    }

    void Report() {
        if (kin_received_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no samples on %s yet. Check IN THIS ORDER: (1) the ego is "
                        "LOCALIZED -- ADAPI publishes kinematics only after "
                        "/initialpose3d; (2) ROS_DOMAIN_ID matches Autoware's; (3) the "
                        "publisher still offers BEST_EFFORT as measured -- a RELIABLE "
                        "request against it matches nothing, silently.",
                        sub_kin_->get_topic_name());
            return;
        }
        RCLCPP_INFO(get_logger(),
                    "kinematics=%llu status=%llu published=%llu gear_unknown=%llu "
                    "write_failed=%llu",
                    static_cast<unsigned long long>(kin_received_),
                    static_cast<unsigned long long>(status_received_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(gear_unknown_),
                    static_cast<unsigned long long>(write_failed_));
        if (status_received_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no /api/vehicle/status yet -- steering_angle is 0 and gear is "
                        "UNKNOWN in every sample published so far.");
        }
    }

    rclcpp::Subscription<autoware_adapi_v1_msgs::msg::VehicleKinematics>::SharedPtr sub_kin_;
    rclcpp::Subscription<autoware_adapi_v1_msgs::msg::VehicleStatus>::SharedPtr sub_status_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::unique_ptr<EgoStateWriter> writer_;

    std::string run_id_;
    AutowareStateSample sample_;

    std::uint64_t seq_ = 0;
    std::uint64_t kin_received_ = 0;
    std::uint64_t status_received_ = 0;
    std::uint64_t published_ = 0;
    std::uint64_t gear_unknown_ = 0;
    std::uint64_t write_failed_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);

    // OPT-IN GUARD. This bridge does not belong in either real configuration
    // (MORAI / Web.Auto + Autoware + MMF + SiLS): there the ego state comes from the PLANT
    // adapter, and running this one alongside it puts TWO publishers on sils/ego/state.
    // DDS does not object, and the vECU then interleaves samples describing two different
    // vehicles with nothing marking which is which -- silent, and exactly the failure mode
    // this stack keeps getting caught by.
    //
    // Refusing to start without an explicit opt-in makes an accidental inclusion in a
    // deployment script fail LOUDLY at t=0 instead of quietly corrupting a run.
    // See docs/sils/sils-integration-topology.md §3.
    if (GetEnvOr("AW_EGO_STATE_STANDIN", "") != "1") {
        RCLCPP_FATAL(rclcpp::get_logger("aw_ego_state_bridge"),
                     "REFUSING TO START. This bridge is a STAND-IN for a missing plant, not "
                     "part of any real configuration -- in MORAI / Web.Auto setups the plant "
                     "adapter owns sils/ego/state, and running both puts two publishers on "
                     "it. If you really are exercising an ECU against Autoware with no plant, "
                     "set AW_EGO_STATE_STANDIN=1. See docs/sils/sils-integration-topology.md.");
        rclcpp::shutdown();
        return 2;
    }

    try {
        rclcpp::spin(std::make_shared<AwEgoStateBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("aw_ego_state_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
