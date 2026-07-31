// SiLS ControlTarget source -- test fixture for the Vehicle Interface (VCU) probe (Option 2
// / Profile B, docs/sils/sils-vehicle-interface-autoware.md).
//
// Stand-in for the Autoware<->DDS bridge's control output: publishes canonical ControlTarget
// samples on the target topic (default autoware/control_target). Lets the VCU be exercised
// WITHOUT Autoware -- vehicle_interface subscribes, its `target_received` fires, and it passes
// the target through as the unified command. Stop this source to see the VCU's no-target
// safe-stop kick in. Open loop; a test fixture, not the real bridge.
//
// Clock modes mirror sils_ego_source (SILS_CLOCK_MODE): wall (default, free-running) and
// lockstep (S3) -- in lockstep it joins the barrier via LockstepClockSource, publishing one
// sim_time-stamped ControlTarget per grant and replying TickComplete("control-source"). The
// target values are constant here, so lockstep just makes their tick alignment reproducible.

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

#include "ControlTargetPubSubTypes.hpp"
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
}  // namespace

int main(int argc, char** argv) {
    const std::string run_id = GetEnvOr("SILS_RUN_ID", "local");
    const std::string topic_name =
        ArgStr(argc, argv, "--topic", GetEnvOr("SILS_CONTROL_TARGET_TOPIC", "autoware/control_target"));
    const int count = ArgInt(argc, argv, "--count", 40);
    // Lockstep only: grants to PARTICIPATE in before self-terminating (== clock_master
    // --ticks; keep > --count so the probe drains the last sample). See sils_ego_source.cc.
    const int part_ticks = ArgInt(argc, argv, "--ticks", count);
    const int rate_ms = ArgInt(argc, argv, "--rate-ms", 100);
    const int startup_ms = ArgInt(argc, argv, "--startup-ms", 1000);
    // Nominal target: mild steering + gentle accel (stand-in for Autoware output).
    const double steer = 0.05;   // rad
    const double accel = 1.0;    // m/s^2

    int domain_id = 0;
    const std::string denv = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!denv.empty()) { try { domain_id = std::stoi(denv); } catch (...) {} }

    DomainParticipantQos pqos;
    pqos.name("sils_control_source:" + run_id);
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(
        domain_id, pqos, nullptr, StatusMask::none());
    if (!participant) {
        std::cerr << "control_source: failed to create participant on domain " << domain_id
                  << std::endl;
        return 2;
    }

    TypeSupport type(new ControlTargetPubSubType());
    type.register_type(participant);
    Publisher* pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(topic_name, type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 256;
    wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataWriter* writer = pub->create_datawriter(topic, wqos, nullptr);

    const std::string clock_mode = GetEnvOr("SILS_CLOCK_MODE", "wall");

    auto publish_target = [&](uint64_t seq, int64_t stamp_ns) {
        ControlTarget m;
        m.run_id(run_id);
        m.seq(seq);
        m.stamp_ns(stamp_ns);
        m.steering_angle(steer);
        m.acceleration(accel);
        m.gear(3);  // DRIVE
        writer->write(&m);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"control_source_sent\",\"seq\":"
                  << seq << ",\"sim_time_ns\":" << stamp_ns << ",\"steering\":" << steer
                  << ",\"acceleration\":" << accel << "}" << std::endl;
    };

    int sent = 0;
    // Hoisted for teardown ordering (delete readers before the grant listener) -- see
    // sils_ego_source.cc.
    std::unique_ptr<sils::LockstepClockSource> clock;
    if (clock_mode == "lockstep") {
        std::signal(SIGINT, OnSig);
        std::signal(SIGTERM, OnSig);
        clock = std::make_unique<sils::LockstepClockSource>(participant, run_id, "control-source", &g_stop);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"control_source_start\",\"topic\":\""
                  << topic_name << "\",\"mode\":\"lockstep\",\"count\":" << count << "}" << std::endl;
        sils::TickInfo tick;
        while (clock->WaitForNextTick(tick)) {
            // Publish first `count` grants; COMPLETE every granted tick until stop (a barrier
            // participant can't leave mid-run, and trailing empty ticks let the probe drain
            // the last sample). Run clock_master with --ticks a bit above --count. See
            // sils_ego_source.cc.
            if (count <= 0 || tick.seq <= static_cast<uint64_t>(count)) {
                publish_target(tick.seq, tick.sim_time_ns);
                ++sent;
            }
            clock->TickDone(tick);  // TickComplete(seq, "control-source") + tick_complete trace
            if (part_ticks > 0 && tick.seq >= static_cast<uint64_t>(part_ticks)) break;  // self-terminate
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));
        std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"control_source_start\",\"topic\":\""
                  << topic_name << "\",\"mode\":\"wall\",\"count\":" << count << ",\"rate_ms\":"
                  << rate_ms << "}" << std::endl;
        for (int i = 1; i <= count; ++i) {
            const int64_t stamp_ns =
                static_cast<int64_t>(i) * static_cast<int64_t>(rate_ms) * 1000000LL;
            publish_target(static_cast<uint64_t>(i), stamp_ns);
            ++sent;
            std::this_thread::sleep_for(std::chrono::milliseconds(rate_ms));
        }
    }

    std::cout << "{\"run_id\":\"" << run_id << "\",\"event\":\"control_source_done\",\"sent\":" << sent
              << "}" << std::endl;

    participant->delete_contained_entities();  // readers first (grant reader references the listener)
    clock.reset();                              // then the clock (grant listener/queue)
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
