// SiLS EgoState source -- test fixture for the vehicle plant-coupling probe (Option 2,
// docs/sils-morai-plant-adapter-design.md §5).
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
EgoStateVec3 Vec3(double x, double y, double z) {
    EgoStateVec3 v;
    v.x(x); v.y(y); v.z(z);
    return v;
}
}  // namespace

int main(int argc, char** argv) {
    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    const std::string topic_name =
        ArgStr(argc, argv, "--topic", GetEnvOr("SILS_EGO_STATE_TOPIC", "sils/ego/state"));
    const int count = ArgInt(argc, argv, "--count", 20);
    const int rate_ms = ArgInt(argc, argv, "--rate-ms", 100);
    const int startup_ms = ArgInt(argc, argv, "--startup-ms", 1000);
    const double accel = 0.5;  // m/s^2 nominal ramp so vx (and position) visibly evolve

    int domain_id = 0;
    const std::string denv = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!denv.empty()) { try { domain_id = std::stoi(denv); } catch (...) {} }

    DomainParticipantQos pqos;
    pqos.name("sils_ego_source:" + run_id);
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
    auto publish_state = [&](uint64_t seq, int64_t stamp_ns, double px, double vx) {
        EgoState s;
        s.run_id(run_id);
        s.seq(seq);
        s.stamp_ns(stamp_ns);
        s.position(Vec3(px, 0.0, 0.0));
        s.orientation_rpy(Vec3(0.0, 0.0, 0.0));
        s.velocity(Vec3(vx, 0.0, 0.0));
        s.acceleration(Vec3(accel, 0.0, 0.0));
        s.yaw_rate(0.0);
        s.steering_angle(0.0);
        s.gear(3);  // DRIVE
        writer->write(&s);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_sent\",\"seq\":" << seq
                  << ",\"sim_time_ns\":" << stamp_ns << ",\"vx\":" << vx << ",\"px\":" << px << "}"
                  << std::endl;
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
            const double t = static_cast<double>(tick.sim_time_ns) / 1e9;  // sim seconds
            const double vx = accel * t;             // exact integral -> pure fn of sim_time
            const double px = 0.5 * accel * t * t;
            publish_state(tick.seq, tick.sim_time_ns, px, vx);
            clock->TickDone(tick);  // TickComplete(seq, "ego-source") + tick_complete trace
            ++sent;
            if (count > 0 && tick.seq >= static_cast<uint64_t>(count)) break;  // count == --ticks
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_start\",\"topic\":\""
                  << topic_name << "\",\"mode\":\"wall\",\"count\":" << count << ",\"rate_ms\":"
                  << rate_ms << "}" << std::endl;
        const double dt = static_cast<double>(rate_ms) / 1000.0;
        double px = 0.0, vx = 0.0;
        for (int i = 1; i <= count; ++i) {
            vx += accel * dt;   // ramp speed
            px += vx * dt;      // integrate position
            const int64_t stamp_ns =
                static_cast<int64_t>(i) * static_cast<int64_t>(rate_ms) * 1000000LL;
            publish_state(static_cast<uint64_t>(i), stamp_ns, px, vx);
            ++sent;
            std::this_thread::sleep_for(std::chrono::milliseconds(rate_ms));
        }
    }

    std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"ego_source_done\",\"sent\":" << sent
              << "}" << std::endl;

    participant->delete_contained_entities();  // readers first (grant reader references the listener)
    clock.reset();                              // then the clock (grant listener/queue)
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
