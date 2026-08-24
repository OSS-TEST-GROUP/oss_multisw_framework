// SiLS EgoState source -- test fixture for the vehicle plant-coupling probe (Option 2,
// docs/sils/sils-morai-plant-adapter-design.md §5).
//
// Stand-in for the sim adapter's STATE broadcast: publishes canonical EgoState samples on
// the `in` topic (default sils/ego/state), on the run's DDS domain. It lets the round trip
// be observed WITHOUT MORAI -- sils_vecu_vehicle_probe subscribes, its `state_received`
// fires, and its P-controller reacts to the evolving speed. OPEN LOOP: it does not read the
// probe's VehicleControl back (a real sim/plant would); vx just ramps so the state is
// visibly alive. A test fixture, not a production component -- the MORAI/AWSIM adapter is
// the real STATE producer.
//
// Two clock modes (SILS_CLOCK_MODE), mirroring the probe:
//   - wall (default, S1/S2): free-running, --rate-ms cadence, wall-time stamps. Staging
//     (S2) pins the consumption ORDER, but which sample a tick sees depends on wall timing.
//   - lockstep (S3): the fixture becomes a BARRIER PARTICIPANT via the same
//     LockstepClockSource the probe uses -- on each TickGrant it derives EgoState as a pure
//     function of sim_time_ns (not wall integration) and replies TickComplete. State VALUES
//     are then reproducible across runs (determinism recovered), not just consumption order.
//     This is the contract a real MORAI/AWSIM adapter implements to join the lockstep loop.
//     Single-barrier lockstep => the probe consumes this state with a deterministic one-tick
//     lag (it freezes tick T's inputs from what the barrier closed by grant T = tick T-1).

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "EgoStatePubSubTypes.hpp"
#include "sils_participant_qos.hpp"
#include "clock_source.hpp"  // sils::LockstepClockSource (reused; TickControl via sils_vecu)

using namespace eprosima::fastdds::dds;

namespace {
std::atomic<bool> g_stop{false};
void OnSig(int) { g_stop.store(true); }

std::string GetEnvOr(const char* n, const std::string& fb) {
    const char* v = std::getenv(n);
    return v ? std::string(v) : fb;
}
int ArgInt(int argc, char** argv, const std::string& flag, int fb) {
    for (int i = 1; i < argc - 1; ++i)
        if (flag == argv[i]) { try { return std::stoi(argv[i + 1]); } catch (...) { return fb; } }
    return fb;
}
std::string ArgStr(int argc, char** argv, const std::string& flag, const std::string& fb) {
    for (int i = 1; i < argc - 1; ++i)
        if (flag == argv[i]) return argv[i + 1];
    return fb;
}
double ArgDbl(int argc, char** argv, const std::string& flag, double fb) {
    for (int i = 1; i + 1 < argc; ++i)
        if (flag == argv[i]) { try { return std::stod(argv[i + 1]); } catch (...) { return fb; } }
    return fb;
}
EgoStateVec3 Vec3(double x, double y, double z) {
    EgoStateVec3 v;
    v.x(x); v.y(y); v.z(z);
    return v;
}

// Speed and distance at sim-time t under "ramp at `accel` until `vmax`, then hold".
// A PURE FUNCTION OF t, deliberately: the lockstep mode derives state from sim_time rather
// than integrating, which is the whole reason its runs are reproducible. Capping by
// clamping an integrated variable instead would reintroduce path dependence.
// vmax <= 0 means no cap, which is the original uncapped ramp.
struct Motion { double vx; double s; };
Motion MotionAt(double t, double accel, double vmax) {
    if (t < 0.0) t = 0.0;
    if (vmax <= 0.0 || accel <= 0.0) return {accel * t, 0.5 * accel * t * t};
    const double t_cap = vmax / accel;
    if (t <= t_cap) return {accel * t, 0.5 * accel * t * t};
    return {vmax, 0.5 * accel * t_cap * t_cap + vmax * (t - t_cap)};
}
}  // namespace

int main(int argc, char** argv) {
    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    const std::string topic_name =
        ArgStr(argc, argv, "--topic", GetEnvOr("SILS_EGO_STATE_TOPIC", "sils/ego/state"));
    const int count = ArgInt(argc, argv, "--count", 20);
    // Lockstep only: how many grants to PARTICIPATE in (complete) before self-terminating.
    // Set == clock_master --ticks so the source leaves exactly when the master stops (no
    // barrier timeout); keep it > --count so the probe (1-tick lag) drains the last sample
    // (no staging tail). Default = count (stops at count; may leave a 1-sample tail).
    const int part_ticks = ArgInt(argc, argv, "--ticks", count);
    const int rate_ms = ArgInt(argc, argv, "--rate-ms", 100);
    const int startup_ms = ArgInt(argc, argv, "--startup-ms", 1000);
    const double accel = 0.5;  // m/s^2 nominal ramp so vx (and position) visibly evolve

    // --- where on the map, and pointing which way ---------------------------------
    //
    // Added for stage 3 (docs/sils/evidence/vehicle-seat-no-simulator-2026-08-11.md §5.1).
    // Without these the fixture starts at (0,0) heading +x, which is the MAP ORIGIN and is
    // not on any lanelet. Autoware then localizes nowhere, refuses to plan a route, and
    // "Control is skipped since input data is not ready" never stops -- which looks like a
    // bridge defect and is not one. Give it a pose the map actually contains and Autoware
    // plans against our bridges' data.
    //
    // DEFAULTS ARE THE OLD BEHAVIOUR EXACTLY: at x0=y0=yaw0=0 the position below reduces to
    // (s, 0, 0) because cos(0)=1 and sin(0)=0, and orientation stays zero. Existing traces
    // and goldens are unaffected.
    //
    // Coordinates are Autoware's `map` frame, which for an MGRS map is UTM modulo 100000.
    // On the sample-map-planning rig a known-good start is
    //     --x0 3638.69 --y0 73605.25 --yaw0 -1.4441
    // (from /workspace/log/pose.json, the pose used to drive the partition on 08-10).
    const double x0 = ArgDbl(argc, argv, "--x0", 0.0);
    const double y0 = ArgDbl(argc, argv, "--y0", 0.0);
    const double yaw0 = ArgDbl(argc, argv, "--yaw0", 0.0);

    // Speed cap, m/s. 0 keeps the old uncapped ramp -- which is fine for a 2-second unit
    // run and useless for a 300-second one: at 0.5 m/s^2 the fixture is doing 150 m/s by
    // then and has left the map entirely. Capping keeps it a pure function of sim_time (see
    // Travelled below), so lockstep determinism survives.
    const double vmax = ArgDbl(argc, argv, "--vmax", 0.0);

    int domain_id = 0;
    const std::string denv = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!denv.empty()) { try { domain_id = std::stoi(denv); } catch (...) {} }

    DomainParticipantQos pqos;
    pqos.name("sils_ego_source:" + run_id);

    sils::ApplySilsParticipantQos(pqos);   // see sils_participant_qos.hpp

    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(
        domain_id, pqos, nullptr, StatusMask::none());
    if (!participant) {
        std::cerr << "ego_source: failed to create participant on domain " << domain_id << std::endl;
        return 2;
    }

    TypeSupport type(new EgoStatePubSubType());
    type.register_type(participant);
    Publisher* pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(topic_name, type.get_type_name(), TOPIC_QOS_DEFAULT);

    // Match the probe's reader QoS (RELIABLE + KEEP_LAST + TRANSIENT_LOCAL) so RxO matches
    // and a late-joining probe still gets recent samples.
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 256;
    wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataWriter* writer = pub->create_datawriter(topic, wqos, nullptr);

    const std::string clock_mode = GetEnvOr("SILS_CLOCK_MODE", "wall");

    // Build + publish one EgoState (shared by both clock modes). seq/stamp/kinematics are
    // supplied by the caller so the wall loop can integrate wall-dt while lockstep derives
    // everything from sim_time.
    // `travelled` is arc length along the heading, not an x coordinate. The map position is
    // derived from it here so both clock modes share one definition of "where the car is".
    auto publish_state = [&](uint64_t seq, int64_t stamp_ns, double travelled, double vx) {
        const double px = x0 + travelled * std::cos(yaw0);
        const double py = y0 + travelled * std::sin(yaw0);
        EgoState s;
        s.run_id(run_id);
        s.seq(seq);
        s.stamp_ns(stamp_ns);
        s.position(Vec3(px, py, 0.0));
        s.orientation_rpy(Vec3(0.0, 0.0, yaw0));
        // Velocity and acceleration are VEHICLE frame (the IDL says so), so they do not
        // rotate with yaw0 -- the car is always moving along its own +x. Rotating them here
        // would double-apply the heading, since consumers already read them as body frame.
        s.velocity(Vec3(vx, 0.0, 0.0));
        // Acceleration goes to zero once the speed cap is reached; reporting the nominal
        // ramp forever would tell Autoware the car is still accelerating while it holds a
        // constant speed, and the longitudinal controller acts on measured acceleration.
        s.acceleration(Vec3((vmax > 0.0 && vx >= vmax) ? 0.0 : accel, 0.0, 0.0));
        s.yaw_rate(0.0);
        s.steering_angle(0.0);
        s.gear(3);  // DRIVE
        writer->write(&s);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_sent\",\"seq\":" << seq
                  << ",\"sim_time_ns\":" << stamp_ns << ",\"vx\":" << vx << ",\"px\":" << px
                  << ",\"py\":" << py << "}" << std::endl;
    };

    int sent = 0;
    // Hoisted so teardown can delete the participant's readers BEFORE this (and its grant
    // listener) is destroyed -- same ordering the probe uses (delete_contained_entities then
    // reset), else an orphaned grant reader could call into a freed listener.
    std::unique_ptr<sils::LockstepClockSource> clock;
    if (clock_mode == "lockstep") {
        // S3: join the lockstep barrier via the SAME LockstepClockSource the probe uses.
        // On each grant, state is a pure function of sim_time -> reproducible across runs.
        std::signal(SIGINT, OnSig);
        std::signal(SIGTERM, OnSig);
        clock = std::make_unique<sils::LockstepClockSource>(participant, run_id, "ego-source", &g_stop);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_start\",\"topic\":\""
                  << topic_name << "\",\"mode\":\"lockstep\",\"count\":" << count << "}" << std::endl;
        sils::TickInfo tick;
        while (clock->WaitForNextTick(tick)) {
            // Publish only the first `count` grants, but COMPLETE every granted tick until
            // stop -- a barrier participant must not leave while the master is still
            // granting (else the barrier times out). Completing a few trailing empty ticks
            // also lets the probe (1-tick lag) drain the last published sample, so nothing
            // is left unstaged. Run clock_master with --ticks a bit above --count.
            if (count <= 0 || tick.seq <= static_cast<uint64_t>(count)) {
                const double t = static_cast<double>(tick.sim_time_ns) / 1e9;  // sim seconds
                const Motion m = MotionAt(t, accel, vmax);  // pure fn of sim_time
                publish_state(tick.seq, tick.sim_time_ns, m.s, m.vx);
                ++sent;
            }
            clock->TickDone(tick);  // TickComplete(seq, "ego-source") + tick_complete trace
            if (part_ticks > 0 && tick.seq >= static_cast<uint64_t>(part_ticks)) break;  // self-terminate
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_start\",\"topic\":\""
                  << topic_name << "\",\"mode\":\"wall\",\"count\":" << count << ",\"rate_ms\":"
                  << rate_ms << "}" << std::endl;
        const double dt = static_cast<double>(rate_ms) / 1000.0;
        double travelled = 0.0, vx = 0.0;

        // ★ PACE AGAINST AN ABSOLUTE DEADLINE, NOT sleep_for.
        //
        // The stamp below is i * rate_ms -- sim time advances by exactly rate_ms per
        // sample, by construction. `sleep_for(rate_ms)` AFTER doing the work makes the WALL
        // period rate_ms + work, so sim time falls behind the wall by the work time on
        // every single sample. Measured at --rate-ms 20: 20.4 ms actual, a 2% slip, i.e.
        // ~20 ms of lag per second.
        //
        // That is not cosmetic. The Autoware bridges anchor sim time to the wall clock once
        // and then watch the offset (adapters/ss2/include/aw_stamp_policy.hpp); a 2% slip
        // blows the 20 ms tolerance after one second and leaves the watchdog permanently
        // red -- which trains everyone to ignore the one alarm that catches real clock
        // faults. Measured against SS_v2 the genuine drift is 6.8e-07 s/s, five orders of
        // magnitude smaller, so this fixture was the outlier, not the plant.
        //
        // sleep_until absorbs the work time instead of adding to it. The stamp is still a
        // pure function of i, so determinism is untouched.
        auto next = std::chrono::steady_clock::now();
        for (int i = 1; i <= count; ++i) {
            vx += accel * dt;                            // ramp speed
            if (vmax > 0.0 && vx > vmax) vx = vmax;      // ...up to the cap
            travelled += vx * dt;                        // integrate distance along heading
            const int64_t stamp_ns =
                static_cast<int64_t>(i) * static_cast<int64_t>(rate_ms) * 1000000LL;
            publish_state(static_cast<uint64_t>(i), stamp_ns, travelled, vx);
            ++sent;
            next += std::chrono::milliseconds(rate_ms);
            std::this_thread::sleep_until(next);
        }
    }

    std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_done\",\"sent\":" << sent
              << "}" << std::endl;

    participant->delete_contained_entities();  // readers first (grant reader references the listener)
    clock.reset();                              // then the clock (grant listener/queue)
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
