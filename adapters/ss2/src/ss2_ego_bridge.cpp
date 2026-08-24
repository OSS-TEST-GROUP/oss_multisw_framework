// ss2_ego_bridge -- scenario_simulator_v2 ROS 2 state -> canonical SiLS EgoState on DDS.
// Loop B, work item B2-1.  Docs: adapters/ss2/README.md
//
// This node is the ANTI-CORRUPTION LAYER and it lives OUTSIDE service/sils/ on purpose:
// it is the only thing that changes when the simulator changes, and SiLS must not.
// See docs/sils/sils-simulator-loops.md §3 (switch point).
//
// Shape (BR-C-2): an rclcpp node takes the ROS 2 side and re-publishes with the NATIVE
// Fast DDS API using our canonical type.  It is not a "ROS 2 to ROS 2" relay -- our vECU
// is a bare DDS participant and would never match a ROS-mangled topic
// (`rt/...` + `pkg::msg::dds_::Type_`); the measured probe report shows exactly that
// naming.  Bridging is therefore mandatory, not stylistic.
//
// STAGE 1 ONLY -- open loop.  This publishes state INTO the bus; nothing feeds control
// back to the simulator.  Closing the loop needs the concealer decision (B1-2) and, for
// determinism, the frame-drive patch (W3/B2-2).  Do not extend this file into a control
// path without those.
//
//     SS_v2 /entity/status ──▶ [this node] ──▶ sils/ego/state ──▶ vECU ──▶ trace ──▶ gate
//                                                                  (control output dropped)

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <traffic_simulator_msgs/msg/entity_status_with_trajectory_array.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Generated from idl/dds/sils/EgoState.idl by fastddsgen at build time.  The generator
// changed its header extension across major versions -- v4.x (for Fast DDS 3.x) emits
// .hpp, v3.x (for Fast DDS 2.x, which ROS 2 Humble bundles) emits .h -- so resolve it at
// compile time rather than pinning one and breaking on the other.  This cannot be decided
// at CMake configure time: the files do not exist yet when CMake runs.
#if __has_include("EgoStatePubSubTypes.hpp")
#  include "EgoStatePubSubTypes.hpp"
#else
#  include "EgoStatePubSubTypes.h"
#endif

#include "ss2_ego_mapping.hpp"
#include "aw_type_propagation.hpp"

namespace {

using namespace eprosima::fastdds::dds;
using sils::ss2::CanonicalEgoState;
using sils::ss2::EgoStateProvenance;
using sils::ss2::Ss2Entity;
using sils::ss2::SynthesizedDefaults;

std::string GetEnvOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

// DataWriter::write(void*) returns `bool` on Fast DDS 2.x -- which is what this container
// has (measured: ros-humble-fastrtps 2.6.11) -- and `ReturnCode_t` on 3.x. An overload
// pair covers both; only the matching one is ever selected.
inline bool WriteOk(bool r) { return r; }

// Fast DDS 3.x path.
//
// The comparison MUST stay dependent on the template parameter. Writing
// `eprosima::fastdds::dds::RETCODE_OK` here does not work: a non-dependent qualified name
// is looked up when the template is DEFINED, not when it is instantiated, so on 2.x --
// where that constant does not exist -- it is a hard error even though this overload is
// never selected. (That is exactly how this file failed to build the first time.)
//
// RETCODE_OK is the zero value in both lines, so a value-initialized ReturnCodeT is the
// same thing and needs no name lookup at all.
//
// Untested in practice: this container is 2.x, so only the bool overload runs today.
template <typename ReturnCodeT>
inline bool WriteOk(const ReturnCodeT& r) {
    return r == ReturnCodeT{};
}

EgoStateVec3 MakeVec3(const sils::ss2::Vec3& v) {
    EgoStateVec3 out;
    out.x(v.x);
    out.y(v.y);
    out.z(v.z);
    return out;
}

// --- DDS side ----------------------------------------------------------------
//
// QoS mirrors service/sils/vecu/src/sils_ego_source.cc (the fixture writer this replaces
// on the wire) and vehicle_run.hpp's reader: RELIABLE + KEEP_LAST(256), TRANSIENT_LOCAL
// unless SILS_DURABILITY=volatile.  It must track that env var with the vECU -- a
// TRANSIENT_LOCAL reader against a VOLATILE writer does not match, and an RxO mismatch
// shows up as silence, not as an error.
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

// --- the node ----------------------------------------------------------------

class Ss2EgoBridge : public rclcpp::Node {
public:
    Ss2EgoBridge() : rclcpp::Node("ss2_ego_bridge") {
        // Defaults come from the environment where SiLS already has a convention for the
        // name (VISS_DDS_DOMAIN_ID, SILS_RUN_ID, SILS_DURABILITY), and from a ROS param
        // otherwise.  Params win so a launch file can override without touching env.
        // ⚠ THE TOPIC NAME DEPENDS ON WHICH SS_v2 LAUNCHER IS RUNNING -- traffic_simulator
        // publishes "entity/status" as a RELATIVE name, so it resolves against the
        // publishing node's namespace:
        //
        //   cpp_mock_scenarios (ss2-mock)        -> /entity/status
        //   scenario_test_runner (ss2-scenario)  -> /simulation/entity/status
        //
        // The default deliberately stays the stage-1 name: that verdict is a published
        // artefact and must keep reproducing. run_closed_loop.sh and the ss2-adapter
        // compose service pass the namespaced one. Getting it wrong is SILENT -- the
        // subscription simply never matches and the node reports "no samples yet" forever.
        // Full note in docker/compose/docker-compose-sils-ss2.yaml (ss2-adapter).
        const std::string ss2_topic =
            declare_parameter<std::string>("ss2_topic", "/entity/status");
        const std::string sils_topic =
            declare_parameter<std::string>("sils_topic", "sils/ego/state");
        ego_name_ = declare_parameter<std::string>("ego_name", GetEnvOr("SS2_EGO_NAME", ""));
        run_id_ = declare_parameter<std::string>("run_id", GetEnvOr("SILS_RUN_ID", "ss2-wire"));
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        const bool transient_local = declare_parameter<bool>(
            "durability_transient_local", GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile");

        writer_ = std::make_unique<EgoStateWriter>(domain_id, sils_topic, transient_local);

        // Subscription QoS.  MEASURED (docs/sils/evidence/ss2-interop-probe-2026-08-06.md):
        // the /entity/status publisher offers RELIABLE + VOLATILE.  The reader must
        // therefore be VOLATILE -- requesting TRANSIENT_LOCAL against a VOLATILE offer is
        // an RxO violation and simply never matches.  Do not "upgrade" this for late-join.
        rclcpp::QoS qos(rclcpp::KeepLast(10));
        qos.reliable().durability_volatile();

        sub_ = create_subscription<traffic_simulator_msgs::msg::EntityStatusWithTrajectoryArray>(
            ss2_topic, qos,
            [this](traffic_simulator_msgs::msg::EntityStatusWithTrajectoryArray::ConstSharedPtr msg) {
                OnEntityStatus(*msg);
            });

        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "ss2_ego_bridge: %s (ROS 2) -> %s (DDS domain %d, %s), run_id=%s, ego=%s",
                    ss2_topic.c_str(), sils_topic.c_str(), domain_id,
                    transient_local ? "TRANSIENT_LOCAL" : "VOLATILE", run_id_.c_str(),
                    ego_name_.empty() ? "<first EGO-typed>" : ego_name_.c_str());

        // Provenance banner.  SS_v2 does not publish steering angle or gear; saying so
        // once, loudly, is what keeps a later reader of a trace from believing they were
        // measured.  See README "미제공 필드" and ss2_ego_mapping.hpp EgoStateProvenance.
        EgoStateProvenance probe;
        probe.steering_angle_synthesized = true;
        probe.gear_synthesized = true;
        RCLCPP_WARN(get_logger(),
                    "SYNTHESIZED FIELDS (not published by scenario_simulator_v2): %s "
                    "-- steering_angle=%.3f rad, gear=%u. Runs using this adapter MUST use a "
                    "plant id that marks it (e.g. --plant ss2-wire) so compare_golden's "
                    "plant gate keeps these out of a golden where the fields were real.",
                    probe.summary().c_str(), defaults_.steering_angle,
                    static_cast<unsigned>(defaults_.gear));
    }

private:
    void OnEntityStatus(
        const traffic_simulator_msgs::msg::EntityStatusWithTrajectoryArray& msg) {
        ++received_;

        std::vector<Ss2Entity> entities;
        entities.reserve(msg.data.size());
        for (const auto& d : msg.data) {
            Ss2Entity e;
            e.name = d.status.name.empty() ? d.name : d.status.name;
            e.type = static_cast<sils::ss2::EntityType>(d.status.type.type);
            // EntityStatusWithTrajectory also carries a `time`; the entity's own
            // status.time is the one bound to this kinematic sample, so prefer it and
            // fall back only if it is unset.
            e.time_sec = (d.status.time != 0.0) ? d.status.time : d.time;
            e.position.x = d.status.pose.position.x;
            e.position.y = d.status.pose.position.y;
            e.position.z = d.status.pose.position.z;
            e.orientation.x = d.status.pose.orientation.x;
            e.orientation.y = d.status.pose.orientation.y;
            e.orientation.z = d.status.pose.orientation.z;
            e.orientation.w = d.status.pose.orientation.w;
            e.twist_linear.x = d.status.action_status.twist.linear.x;
            e.twist_linear.y = d.status.action_status.twist.linear.y;
            e.twist_linear.z = d.status.action_status.twist.linear.z;
            e.twist_angular.x = d.status.action_status.twist.angular.x;
            e.twist_angular.y = d.status.action_status.twist.angular.y;
            e.twist_angular.z = d.status.action_status.twist.angular.z;
            e.accel_linear.x = d.status.action_status.accel.linear.x;
            e.accel_linear.y = d.status.action_status.accel.linear.y;
            e.accel_linear.z = d.status.action_status.accel.linear.z;
            entities.push_back(std::move(e));
        }

        const Ss2Entity* ego = sils::ss2::select_ego(entities, ego_name_);
        if (ego == nullptr) {
            // Normal during the first frames before the ego spawns.  Publishing a zero
            // state here would look like a car parked at the map origin, so skip and
            // count instead -- the count is what tells you if it never spawned at all.
            ++skipped_no_ego_;
            // Say WHAT was on the bus, once.  "No ego found, set ego_name" is not
            // actionable without the candidate list: the operator cannot know whether the
            // scenario spawns no ego at all, or spawns one under an unexpected name/type.
            if (!inventory_logged_) {
                inventory_logged_ = true;
                std::string inv;
                for (const auto& e : entities) {
                    if (!inv.empty()) inv += ", ";
                    inv += "'" + e.name + "'(type=" +
                           std::to_string(static_cast<int>(e.type)) + ")";
                }
                RCLCPP_WARN(get_logger(),
                            "entities seen on the bus (%zu): %s  [EntityType: 0=EGO 1=VEHICLE "
                            "2=PEDESTRIAN 3=MISC_OBJECT]. None is type 0; select one by name "
                            "with SS2_EGO_NAME=<name> if the scenario drives a non-EGO entity.",
                            entities.size(), inv.empty() ? "<empty array>" : inv.c_str());
            }
            return;
        }

        // The scenario clock has not started yet -- EntityStatus.time is NaN, which is how
        // traffic_simulator SAYS so (see is_usable_sim_time). Publishing anyway would emit a
        // state whose stamp never moves, and nothing downstream could tell it from a real
        // one. Skip and count, exactly as for a missing ego.
        if (!sils::ss2::is_usable_sim_time(ego->time_sec)) {
            ++skipped_no_time_;
            if (!no_time_logged_) {
                no_time_logged_ = true;
                RCLCPP_WARN(get_logger(),
                            "ego '%s' has no scenario time yet (EntityStatus.time=%f). "
                            "traffic_simulator leaves it NaN until SimulationClock::start(), "
                            "and openscenario_interpreter calls that only once every "
                            "Autoware-controlled entity reports ENGAGED. With no Autoware "
                            "attached it stays NaN for the whole run and nothing is "
                            "published. This is the expected state for an EgoEntity scenario "
                            "run with launch_autoware:=false -- attach Autoware, or use a "
                            "scenario whose entities are not Autoware-controlled.",
                            ego->name.c_str(), ego->time_sec);
            }
            return;
        }

        const CanonicalEgoState s = sils::ss2::map_ego_state(*ego, run_id_, seq_, defaults_);
        if (writer_->Write(s)) {
            ++published_;
            ++seq_;
        } else {
            ++write_failed_;
        }
    }

    void Report() {
        if (received_ == 0) {
            RCLCPP_WARN(get_logger(),
                        "no samples on the SS_v2 topic yet -- check the scenario is running, "
                        "and that ROS_DOMAIN_ID matches the simulator's");
            return;
        }
        if (published_ == 0 && skipped_no_time_ > 0) {
            RCLCPP_WARN(get_logger(),
                        "received=%llu but the scenario clock has not started "
                        "(skipped_no_time=%llu). Nothing is being published. See the "
                        "one-time warning above -- this is normal until Autoware engages "
                        "the ego.",
                        static_cast<unsigned long long>(received_),
                        static_cast<unsigned long long>(skipped_no_time_));
            return;
        }
        if (published_ == 0 && skipped_no_ego_ > 0) {
            RCLCPP_WARN(get_logger(),
                        "received=%llu but no ego selected yet (skipped=%llu). "
                        "If this persists, the scenario may have no EGO-typed entity; "
                        "set ego_name / SS2_EGO_NAME to select by name.",
                        static_cast<unsigned long long>(received_),
                        static_cast<unsigned long long>(skipped_no_ego_));
            return;
        }
        RCLCPP_INFO(get_logger(),
                    "received=%llu published=%llu skipped_no_ego=%llu skipped_no_time=%llu "
                    "write_failed=%llu",
                    static_cast<unsigned long long>(received_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(skipped_no_ego_),
                    static_cast<unsigned long long>(skipped_no_time_),
                    static_cast<unsigned long long>(write_failed_));
    }

    rclcpp::Subscription<
        traffic_simulator_msgs::msg::EntityStatusWithTrajectoryArray>::SharedPtr sub_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    std::unique_ptr<EgoStateWriter> writer_;

    std::string ego_name_;
    std::string run_id_;
    SynthesizedDefaults defaults_{};

    bool inventory_logged_ = false;
    bool no_time_logged_ = false;
    std::uint64_t seq_ = 0;
    std::uint64_t received_ = 0;
    std::uint64_t published_ = 0;
    std::uint64_t skipped_no_ego_ = 0;
    std::uint64_t skipped_no_time_ = 0;
    std::uint64_t write_failed_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<Ss2EgoBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("ss2_ego_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
