// aw_localization_bridge -- canonical SiLS EgoState -> Autoware /localization/*.
// Stage 3, step 6.  Docs: docs/sils/sils-integration-topology.md §3
//
// ★ WHY THIS EXISTS: aw_vehicle_status_bridge IS NOT ENOUGH
//
// The plan was "turn simple_planning_simulator off and see whether Autoware runs on
// aw_vehicle_status_bridge alone". Reading the launch files settles it before running
// anything: it CANNOT. The simulator is not only the /vehicle/status/* publisher.
//
// From autoware_simple_planning_simulator/launch/simple_planning_simulator.launch.py, in
// the mode the partition actually uses (motion_publish_mode=full_motion, which is what you
// get for any localization_sim_mode other than pose_twist_estimator -- ours is `api`):
//
//   output/odometry      -> /localization/kinematic_state      <-- THIS FILE
//   output/acceleration  -> /localization/acceleration         <-- THIS FILE
//   output/twist         -> /vehicle/status/velocity_status    aw_vehicle_status_bridge
//   output/steering      -> /vehicle/status/steering_status            "
//   output/gear_report   -> /vehicle/status/gear_status                "
//   output/control_mode  -> /vehicle/status/control_mode                "
//   output/turn_indicators_report / hazard_lights_report                "
//   output/actuation_status -> /vehicle/status/actuation_status  no consumer here (below)
//   output/imu           -> /sensing/imu/imu_data               AEB only (below)
//
// and obigo_launch/launch/planning_simulator.launch.xml hardcodes
//
//   <arg name="launch_localization" value="false"/>
//
// so with the simulator off NOTHING publishes ego pose. Planning and control both block on
// it. The failure is loud rather than silent for once ("waiting for current_twist msg"),
// but it is still a failure, and it is not a defect in the /vehicle/status/* bridge.
//
// The two we deliberately do NOT publish:
//   - /vehicle/status/actuation_status: consumed by autoware_raw_vehicle_cmd_converter,
//     which simple_planning_simulator.launch.py launches only when vehicle_model_type
//     contains "ACTUATION_CMD". sample_vehicle's simulator_model.param.yaml says
//     DELAY_STEER_ACC_GEARED, so that converter never runs here and nothing subscribes.
//   - /sensing/imu/imu_data: subscribed by autoware_autonomous_emergency_braking
//     (tier4_control_launch/launch/control.launch.xml:219) and by eagleye, which is not
//     running. MEASURED 2026-08-11: AEB does NOT block on it. Given ego velocity it stops
//     saying "waiting for ego velocity" and moves on to "waiting for object pointcloud
//     message" without ever asking for the IMU.
//
// ★ WHEN THIS NODE IS WRONG TO RUN
//
// It is not scaffolding (unlike aw_ego_state_bridge) -- both real configurations, MORAI and
// Web.Auto, need it, because both run the partition in planning_simulation mode where
// Autoware's own localization is off and the plant is the only thing that knows where the
// car is.
//
// But it IS configuration-dependent. Point Autoware at real sensors with
// launch_localization=true and this node must be OFF: Autoware would then have two sources
// of truth for ego pose, its own estimator and ours, and the one that loses is decided by
// publication order. There is no runtime guard for that -- the startup banner says it, and
// §3 of the topology doc says it.
//
// ★ QoS MEASURED 2026-08-11 on the live Control partition (domain 43), with the vendor
//   simulator still running so there was something to measure:
//
//     /localization/kinematic_state  nav_msgs/Odometry                    28 subscribers
//     /localization/acceleration     geometry_msgs/AccelWithCovarianceStamped  10
//
//   both RELIABLE, KEEP_LAST(1), VOLATILE -- which is what the defaults below already
//   were. Raw capture: docs/sils/evidence/sps-topic-inventory-2026-08-11.txt.
//
//   38 subscribers here against 14 on all six /vehicle/status/* topics combined. The half
//   of the problem this file exists for is the BIGGER half, which is exactly why "the
//   status bridge is enough" was worth checking before running anything.
//
//   They stay parameters anyway: the numbers above are one partition on one day, and a
//   reader that requests more than we offer never matches and says nothing about it.
//
// ★ AND THE TF, WHICH IS NOT A TOPIC AND SO WAS NOT IN THE INVENTORY
//
// simple_planning_simulator also BROADCASTS map -> base_link. Nothing in a `ros2 topic list`
// audit shows that as a gap, because /tf still has a publisher (robot_state_publisher, for
// base_link -> the sensors) and still has 14 subscribers. It looks healthy and is not:
//
//     $ ros2 run tf2_ros tf2_echo map base_link
//     Could not find a connection between 'map' and 'base_link' because they are not
//     part of the same tree. Tf has two or more unconnected trees.
//
// Downstream this surfaces as "Failed to get transform from object frame to base_link" from
// the collision detector, and operation mode refuses to go AUTONOMOUS ("The target mode is
// not available"). Publishing Odometry is NOT enough: in a sensor-fed Autoware the EKF
// localizer publishes the odometry AND broadcasts the transform, and with
// launch_localization=false nobody does either. So this node does both, from one sample.
// Turn it off with publish_tf:=false if something else owns the transform.
//
// ★ AND /initialpose3d, WHICH IS NOT A POSE INPUT AT ALL IN THIS PARTITION
//
// Until 2026-08-11 this was a manual step: publish /initialpose3d by hand once, or
// operation mode stays unavailable no matter how healthy everything else is. The reason
// was assumed to be "launch_localization=false, so no pose initializer consumes it". That
// assumption was WRONG, and the real mechanism decides how this node has to publish it.
//
// Read out of the vendor image (ghcr.io/oss-test-group/autoware-partition, 2026-08-11):
//
//   - autoware_pose_initializer is NOT INSTALLED in any of the three partition images.
//   - TWO things consume /initialpose3d, in different images, and BOTH gate autonomous:
//
//       planning_sim_localization_state   perception image, a vendor shim (below)
//       topic_state_monitor_initialpose3d decision image, component_state_monitor with
//                                         warn_rate/error_rate/timeout all 0.0, i.e. it
//                                         only ever checks "did one message arrive"
//
//     (A first pass looked for those packages in the CONTROL image only, found nothing,
//     and wrote the monitor off as dead config. It is not: `ros2 topic info -v` on the
//     live rig lists it as a subscriber. Per-image absence is not absence -- the three
//     partitions share one ROS graph.)
//
//   - The shim, obigo_launch/planning_sim_localization_state.py, launched by the
//     PERCEPTION partition (perception_simulator_component.launch.xml:88,
//     launch_localization_state_helper default true):
//
//         subscribes  /initialpose3d                    depth 1, RELIABLE, VOLATILE
//         on ANY message: state = INITIALIZED
//         publishes   /localization/initialization_state  TRANSIENT_LOCAL, 1 Hz
//
//     It ignores the message CONTENT (`def _on_initial_pose(self, _msg)`). ADAPI relays
//     that state to /api/localization/initialization_state, and that is what gates
//     autonomous.
//
// So /initialpose3d here is a BOOLEAN LATCH, not a pose -- neither consumer looks at the
// value. Three consequences for this file:
//
//   1. Publishing it is safe from this node. It cannot teleport the car, because nothing
//      in this configuration applies it -- WITH ONE EXCEPTION: if the vendor simulator is
//      running, autoware_simple_planning_simulator subscribes it as input/initialpose and
//      WOULD teleport. This node must not run alongside the simulator anyway (see the
//      warning above), and run_vehicle_seat_bridges.sh refuses that combination.
//   2. We publish the REAL pose, not a placeholder, even though the shim discards it. The
//      day the partition ships a pose initializer, or somebody runs this against the
//      simulator, a placeholder becomes a wrong initial pose. Sending the pose we already
//      know costs nothing and cannot become wrong.
//   3. It has to REPEAT, not fire once. The shim's subscription is VOLATILE depth 1, so a
//      single publish is only seen by a shim that is already up; and the shim's latch
//      resets to UNINITIALIZED if the perception partition restarts. A slow periodic
//      republish (initialpose_period_ms, default 1 s) re-arms it either way. Set
//      initialpose_period_ms:=0 for publish-once, publish_initialpose:=false for the old
//      manual behaviour.
//      MEASURED 2026-08-11: `docker restart adsw-perception-42` mid-run, no human action,
//      /localization/initialization_state back to INITIALIZED(3) within seconds of the
//      shim reappearing. That is the case publish-once cannot cover.
//
// This belongs to the PARTITION, not to the plant. The latch exists because this Autoware
// runs planning_simulation with launch_localization=false; swapping the plant does not
// change it. So MORAI and Web.Auto need this exactly as SS_v2 and the fixture do -- it is
// not SS_v2 scaffolding. The one configuration that must NOT publish it is a sensor-fed
// Autoware with launch_localization=true, and there this whole node is off anyway (see the
// warning above), which is why the flag rides along with the rest of the node rather than
// being decided separately.
//
// What this does NOT fix: /api/localization/initialize, the ADAPI SERVICE. ADAPI forwards
// it to the internal /localization/initialize service, which the shim does not serve and
// pose_initializer would. That service is what SS_v2's concealer calls, and its
// success=false is the blocker documented in
// docs/sils/evidence/autoware-return-path-2026-08-11.md §9. A topic publish cannot stand
// in for it -- see docs/sils/sils-autoware-partition-vendor-requests.md.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/accel_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_broadcaster.h>

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
#include "aw_state_mapping.hpp"  // rpy_to_quat, Quat

namespace {

using namespace eprosima::fastdds::dds;
using sils::aw::Quat;
using sils::aw::rpy_to_quat;

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
    double px = 0.0, py = 0.0, pz = 0.0;
    double roll = 0.0, pitch = 0.0, yaw = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double ax = 0.0, ay = 0.0, az = 0.0;
    double yaw_rate = 0.0;
};

// --- DDS side ----------------------------------------------------------------
//
// QoS mirrors the writer this consumes (ss2_ego_bridge / a MORAI adapter): RELIABLE +
// KEEP_LAST(256), TRANSIENT_LOCAL unless SILS_DURABILITY=volatile. Tracking that env var is
// mandatory -- a TRANSIENT_LOCAL reader against a VOLATILE writer does not match, silently.
// Same reader as aw_vehicle_status_bridge, reading more of the same sample.
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
        // Drain, do not take one -- same reasoning as aw_vehicle_status_bridge: only the
        // newest pose means anything, and a one-per-callback loop falls behind on a burst.
        while (TakeOk(reader->take_next_sample(&s, &info))) {
            if (!info.valid_data) continue;
            EgoSnapshot snap;
            snap.stamp_ns = s.stamp_ns();
            snap.px = s.position().x();
            snap.py = s.position().y();
            snap.pz = s.position().z();
            snap.roll = s.orientation_rpy().x();
            snap.pitch = s.orientation_rpy().y();
            snap.yaw = s.orientation_rpy().z();
            snap.vx = s.velocity().x();
            snap.vy = s.velocity().y();
            snap.vz = s.velocity().z();
            snap.ax = s.acceleration().x();
            snap.ay = s.acceleration().y();
            snap.az = s.acceleration().z();
            snap.yaw_rate = s.yaw_rate();
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

class AwLocalizationBridge : public rclcpp::Node {
public:
    AwLocalizationBridge() : rclcpp::Node("aw_localization_bridge") {
        const std::string sils_topic = declare_parameter<std::string>(
            "sils_topic", "sils/ego/state");
        const std::string odom_topic = declare_parameter<std::string>(
            "odometry_topic", "/localization/kinematic_state");
        const std::string accel_topic = declare_parameter<std::string>(
            "acceleration_topic", "/localization/acceleration");
        // Odometry splits its frames: the pose is in `map`, the twist in the body frame
        // named by child_frame_id. EgoState declares exactly that split (position "map
        // frame", velocity "vehicle frame"), so this is a rename, not a transform.
        map_frame_ = declare_parameter<std::string>("map_frame", "map");
        base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
        const int domain_id = declare_parameter<int>(
            "domain_id", std::atoi(GetEnvOr("VISS_DDS_DOMAIN_ID", "0").c_str()));
        const bool transient_local = declare_parameter<bool>(
            "durability_transient_local",
            GetEnvOr("SILS_DURABILITY", "transient-local") != "volatile");

        // MEASURED 2026-08-11: both topics are RELIABLE, KEEP_LAST(1), VOLATILE. Kept as
        // parameters so a partition that differs can be matched without a rebuild.
        const bool reliable = declare_parameter<bool>("publish_reliable", true);
        const int depth = declare_parameter<int>("publish_depth", 1);
        rclcpp::QoS qos(rclcpp::KeepLast(static_cast<size_t>(depth > 0 ? depth : 1)));
        if (reliable) {
            qos.reliable();
        } else {
            qos.best_effort();
        }
        qos.durability_volatile();

        pub_odom_ = create_publisher<nav_msgs::msg::Odometry>(odom_topic, qos);
        pub_accel_ = create_publisher<geometry_msgs::msg::AccelWithCovarianceStamped>(
            accel_topic, qos);

        // See the TF note at the top. On by default because with launch_localization=false
        // nobody else broadcasts map -> base_link, and the absence does not look like an
        // absence in any topic-level audit.
        // Must match aw_vehicle_status_bridge's policy. Autoware time-aligns this pose
        // against that node's velocity report; two different epochs on the two halves of
        // one sample is worse than either epoch consistently. Default and env var are the
        // same, so they agree unless someone overrides exactly one. See aw_stamp_policy.hpp.
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

        publish_tf_ = declare_parameter<bool>("publish_tf", true);
        if (publish_tf_) {
            tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
        }

        // See the /initialpose3d note at the top: a boolean latch for the vendor's
        // planning_sim_localization_state shim, not a pose input. On by default because
        // without it operation mode never becomes available and the omission looks like
        // anything BUT a missing topic -- it surfaces as "The target mode is not available".
        publish_initialpose_ = declare_parameter<bool>("publish_initialpose", true);
        const std::string initialpose_topic = declare_parameter<std::string>(
            "initialpose_topic", "/initialpose3d");
        const int initialpose_period_ms = declare_parameter<int>("initialpose_period_ms", 1000);
        if (publish_initialpose_) {
            // Matches the shim's subscription (rclpy depth 1 default: RELIABLE, VOLATILE).
            rclcpp::QoS ipqos(rclcpp::KeepLast(1));
            ipqos.reliable();
            ipqos.durability_volatile();
            pub_initialpose_ =
                create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
                    initialpose_topic, ipqos);
            if (initialpose_period_ms > 0) {
                initialpose_timer_ = create_wall_timer(
                    std::chrono::milliseconds(initialpose_period_ms),
                    [this] { PublishInitialPose(); });
            }
        }

        reader_ = std::make_unique<EgoStateReader>(
            domain_id, sils_topic, transient_local,
            [this](const EgoSnapshot& s) { OnEgoState(s); });

        report_timer_ = create_wall_timer(std::chrono::seconds(5), [this] { Report(); });

        RCLCPP_INFO(get_logger(),
                    "aw_localization_bridge: %s (DDS domain %d, %s) -> %s + %s "
                    "(ROS 2 domain %s), frames %s/%s",
                    sils_topic.c_str(), domain_id,
                    transient_local ? "TRANSIENT_LOCAL" : "VOLATILE",
                    odom_topic.c_str(), accel_topic.c_str(),
                    GetEnvOr("ROS_DOMAIN_ID", "0").c_str(),
                    map_frame_.c_str(), base_frame_.c_str());

        RCLCPP_WARN(get_logger(),
                    "RUN THIS ONLY IN planning_simulation MODE. It supplies the ego pose "
                    "that autoware_simple_planning_simulator would, for a rig where the "
                    "plant (MORAI / Web.Auto) is the only thing that knows where the car "
                    "is. If Autoware is launched with launch_localization=true it estimates "
                    "its own pose and this node becomes a SECOND writer on "
                    "/localization/kinematic_state -- publication order decides which one "
                    "Autoware believes, and nothing reports the conflict.");
        RCLCPP_INFO(get_logger(),
                    "publish QoS %s, KEEP_LAST(%d), VOLATILE -- matches what "
                    "simple_planning_simulator offered on the live partition (measured "
                    "2026-08-11). If a partition differs, override rather than assume: a "
                    "reader that requests more than we offer never matches, and says "
                    "nothing about it.",
                    reliable ? "RELIABLE" : "BEST_EFFORT", depth > 0 ? depth : 1);
        if (publish_initialpose_) {
            RCLCPP_INFO(get_logger(),
                        "also publishing %s every %d ms (0 = once, at the first sample). In "
                        "this partition that topic is a BOOLEAN LATCH, not a pose input: the "
                        "only consumer is the vendor shim planning_sim_localization_state, "
                        "which flips /localization/initialization_state to INITIALIZED on any "
                        "message and discards the content. Without it operation mode never "
                        "becomes available. TURN IT OFF (publish_initialpose:=false) if the "
                        "vendor simulator is running -- simple_planning_simulator DOES apply "
                        "this topic and would teleport the car every period.",
                        initialpose_topic.c_str(), initialpose_period_ms);
        } else {
            RCLCPP_WARN(get_logger(),
                        "publish_initialpose:=false -- /initialpose3d is somebody else's job "
                        "now. If nobody does it, every other diagnostic passes and operation "
                        "mode still refuses AUTONOMOUS with 'The target mode is not "
                        "available'.");
        }
        RCLCPP_WARN(get_logger(),
                    "covariance is published as all-zero on both topics. EgoState carries "
                    "no uncertainty, and inventing one would be a fabricated measurement. "
                    "Zero reads as 'unknown' to the consumers that look; a consumer that "
                    "gates on covariance will need a real plant that reports it.");
    }

private:
    void OnEgoState(const EgoSnapshot& s) {
        ++received_;

        // The SOURCE's timestamp, not now(). Autoware time-aligns pose against the velocity
        // report from aw_vehicle_status_bridge, which stamps the same way; stamping either
        // with arrival time would desynchronise the two views of one sample.
        //
        // Under epoch-anchored the source stamp is SHIFTED by a constant latched at the
        // first sample, never re-timed -- spacing, and therefore plant latency, survives.
        std::int64_t stamp_ns = static_cast<std::int64_t>(s.stamp_ns);
        if (stamp_policy_ == sils::ss2::StampPolicy::kEpochAnchored) {
            stamp_ns = anchor_.Apply(stamp_ns, now().nanoseconds());
            // See aw_vehicle_status_bridge: the warm-up samples are the plant's history,
            // not its state. Replaying them as the ego pose would teleport the car.
            if (!anchor_.latched()) {
                ++warmup_dropped_;
                return;
            }
        }
        const rclcpp::Time stamp(stamp_ns);

        nav_msgs::msg::Odometry odom;
        odom.header.stamp = stamp;
        odom.header.frame_id = map_frame_;
        odom.child_frame_id = base_frame_;
        odom.pose.pose.position.x = s.px;
        odom.pose.pose.position.y = s.py;
        odom.pose.pose.position.z = s.pz;
        const Quat q = rpy_to_quat(s.roll, s.pitch, s.yaw);
        odom.pose.pose.orientation.x = q.x;
        odom.pose.pose.orientation.y = q.y;
        odom.pose.pose.orientation.z = q.z;
        odom.pose.pose.orientation.w = q.w;
        odom.twist.twist.linear.x = s.vx;
        odom.twist.twist.linear.y = s.vy;
        odom.twist.twist.linear.z = s.vz;
        // Only yaw rate is canonical. Roll/pitch rates stay zero rather than being derived:
        // a numerically differentiated attitude at plant jitter is noise, and planning would
        // read it as real body motion.
        odom.twist.twist.angular.z = s.yaw_rate;
        pub_odom_->publish(odom);

        geometry_msgs::msg::AccelWithCovarianceStamped accel;
        accel.header.stamp = stamp;
        accel.header.frame_id = base_frame_;  // body frame, matching EgoState.acceleration
        accel.accel.accel.linear.x = s.ax;
        accel.accel.accel.linear.y = s.ay;
        accel.accel.accel.linear.z = s.az;
        pub_accel_->publish(accel);

        // Same pose, same stamp, broadcast as the transform. Deriving it here rather than
        // in a second node keeps the two from ever disagreeing about where the car is --
        // a pose and a transform that drift apart is the kind of fault that shows up as
        // inexplicable planning behaviour rather than as an error.
        if (publish_tf_) {
            geometry_msgs::msg::TransformStamped tf;
            tf.header.stamp = stamp;
            tf.header.frame_id = map_frame_;
            tf.child_frame_id = base_frame_;
            tf.transform.translation.x = s.px;
            tf.transform.translation.y = s.py;
            tf.transform.translation.z = s.pz;
            tf.transform.rotation = odom.pose.pose.orientation;
            tf_broadcaster_->sendTransform(tf);
        }

        ++published_;

        // Same pose, same stamp, handed to the /initialpose3d path. Cached rather than
        // published from here: the latch only needs re-arming about once a second, and one
        // publication per sample at 50 Hz would be 50x the traffic for no added effect.
        if (publish_initialpose_) {
            {
                const std::lock_guard<std::mutex> lock(initialpose_mutex_);
                geometry_msgs::msg::PoseWithCovarianceStamped msg;
                msg.header.stamp = stamp;
                msg.header.frame_id = map_frame_;
                msg.pose.pose = odom.pose.pose;  // covariance stays all-zero, as above
                latest_initialpose_ = msg;
            }
            // First sample goes out immediately -- the timer would otherwise hold the whole
            // rig at "not available" for one period after the plant is already healthy.
            if (initialpose_published_ == 0) PublishInitialPose();
        }
    }

    // Runs on the executor thread (timer) and, for the first sample only, on the DDS reader
    // thread. Both take the mutex; the publisher itself is thread-safe.
    void PublishInitialPose() {
        std::optional<geometry_msgs::msg::PoseWithCovarianceStamped> msg;
        {
            const std::lock_guard<std::mutex> lock(initialpose_mutex_);
            msg = latest_initialpose_;
        }
        // Nothing to send before the first sample. Publishing a zero pose here would be a
        // fabricated position, and would latch the state INITIALIZED before the plant has
        // said anything at all.
        if (!msg.has_value()) return;
        pub_initialpose_->publish(*msg);
        ++initialpose_published_;
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
        RCLCPP_INFO(get_logger(),
                    "received=%llu published=%llu warmup_dropped=%llu initialpose=%llu",
                    static_cast<unsigned long long>(received_),
                    static_cast<unsigned long long>(published_),
                    static_cast<unsigned long long>(warmup_dropped_),
                    static_cast<unsigned long long>(initialpose_published_));
        ReportAnchor();
    }

    // Same question, same wording as aw_vehicle_status_bridge::ReportAnchor: the counters
    // above can look perfect while the stamps quietly age out of Autoware's freshness
    // window, so the anchor's health is reported separately.
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
                    "latched at the first sample no longer lands this pose in Autoware's "
                    "freshness window -- and nothing else will say so. Check the plant's "
                    "realtime_factor (must be 1.0) and use_sim_time. If the rig genuinely "
                    "needs a non-unit factor, this policy is the wrong one: publish the "
                    "plant's clock onto Autoware's domain and run it with use_sim_time.",
                    static_cast<unsigned long long>(anchor_.exceedances()),
                    anchor_.max_deviation_ns() / 1e6);
    }

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pub_odom_;
    rclcpp::Publisher<geometry_msgs::msg::AccelWithCovarianceStamped>::SharedPtr pub_accel_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr pub_initialpose_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    rclcpp::TimerBase::SharedPtr initialpose_timer_;
    std::unique_ptr<EgoStateReader> reader_;

    std::mutex initialpose_mutex_;
    std::optional<geometry_msgs::msg::PoseWithCovarianceStamped> latest_initialpose_;
    bool publish_initialpose_ = true;
    // Touched by both the timer thread and, for the first sample, the DDS reader thread.
    std::atomic<std::uint64_t> initialpose_published_{0};

    std::string map_frame_;
    std::string base_frame_;
    bool publish_tf_ = true;
    sils::ss2::StampPolicy stamp_policy_ = sils::ss2::StampPolicy::kEpochAnchored;
    sils::ss2::StampAnchor anchor_{};
    std::uint64_t warmup_dropped_ = 0;
    std::uint64_t received_ = 0;
    std::uint64_t published_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<AwLocalizationBridge>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("aw_localization_bridge"), "fatal: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
