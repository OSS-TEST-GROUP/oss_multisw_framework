#ifndef SILS_VECU_VEHICLE_RUN_HPP
#define SILS_VECU_VEHICLE_RUN_HPP

// sils::RunVehicleProbe -- reusable tick loop for the MORAI/AWSIM plant-coupling path
// (Option 2, bus-coupled world). The bus-coupled counterpart of RunProbe (run_loop.hpp):
// it owns DDS participant/channel-map readers+writers, the free-run tick loop, the JSONL
// trace, and teardown, and calls an INJECTED IVehicleBehavior each tick (state in ->
// control out). A developer's plant-coupled ECU builds its behavior and hands it in --
// same shape as RunProbe taking IBehavior. docs/sils-morai-plant-adapter-design.md §5.
//
// S1 scope: wall clock (free-run), polling take each tick (no staging -- S2), no lockstep
// (S3). Header-only (inline) like run_loop.hpp so it compiles into the single executable
// that includes it.

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include "channel_map.hpp"
#include "clock_source.hpp"
#include "config.hpp"
#include "vehicle_behavior.hpp"
#include "vehicle_wire.hpp"

namespace sils {

inline std::atomic<bool> g_vehicle_stop{false};
inline void OnVehicleSignal(int /*signum*/) { g_vehicle_stop.store(true); }

inline long long VehicleWallMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// JSONL trace, same shape as run_loop.hpp's TraceEvent (conformance kit consumes this).
inline void VehicleTrace(const std::string& run_id, const std::string& instance_id,
                         const std::string& event, const std::string& extra = "") {
    std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << instance_id
              << "\",\"event\":\"" << event << "\",\"wall_time_ms\":" << VehicleWallMs()
              << (extra.empty() ? "" : ("," + extra)) << "}" << std::endl;
}

// Run one plant-coupled probe to completion. `behavior` is required (the injected control
// port). Returns a process exit code (0 ok, 1 DDS setup, 2 config/channel-map fail-fast).
inline int RunVehicleProbe(const ProbeConfig& cfg, std::unique_ptr<IVehicleBehavior> behavior) {
    using namespace eprosima::fastdds::dds;

    std::signal(SIGINT, OnVehicleSignal);
    std::signal(SIGTERM, OnVehicleSignal);

    const std::string& run_id = cfg.run_id;
    const std::string& id = cfg.instance_id;

    if (!behavior) {
        std::cerr << "RunVehicleProbe: a vehicle behavior is required (inject one, e.g. "
                     "std::make_unique<sils::LongitudinalBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint))"
                  << std::endl;
        return 2;
    }
    // Channel-map driven -- fail-closed if none configured (§14): a plant-coupling probe
    // with no channels would silently do nothing.
    if (cfg.channelmap_file.empty()) {
        std::cerr << "RunVehicleProbe: SILS_CHANNELMAP is required (e.g. "
                     "config/sils/channelmap/ego-min.yaml)" << std::endl;
        return 2;
    }
    ChannelMap cmap;
    std::string cmap_err;
    if (!cmap.Load(cfg.channelmap_file, cmap_err)) {
        std::cerr << "RunVehicleProbe: " << cmap_err << std::endl;
        return 2;
    }

    VehicleTrace(run_id, id, "run_start",
                 "\"domain_id\":" + std::to_string(cfg.domain_id) + ",\"mode\":\"vehicle-coupling\"");

    DomainParticipantQos pqos;
    pqos.name("sils_vecu_vehicle_probe:" + id);
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(
        cfg.domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "RunVehicleProbe: failed to create participant on domain " << cfg.domain_id
                  << std::endl;
        return 1;
    }

    VehicleWireCodec codec(run_id);
    codec.RegisterTypes(participant);

    // QoS: RELIABLE + KEEP_LAST (+ TRANSIENT_LOCAL by default) to match a typical adapter/
    // SUT endpoint, mirroring run_loop.hpp's writer QoS.
    auto apply_qos_r = [&](DataReaderQos& q) {
        q.reliability().kind = RELIABLE_RELIABILITY_QOS;
        q.history().kind = KEEP_LAST_HISTORY_QOS;
        q.history().depth = 256;
        if (cfg.durability_transient_local) q.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    };
    auto apply_qos_w = [&](DataWriterQos& q) {
        q.reliability().kind = RELIABLE_RELIABILITY_QOS;
        q.history().kind = KEEP_LAST_HISTORY_QOS;
        q.history().depth = 256;
        if (cfg.durability_transient_local) q.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    };

    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    Publisher* pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

    std::vector<DataReader*> state_readers;
    std::vector<DataWriter*> control_writers;
    VehicleTrace(run_id, id, "channelmap_start",
                 "\"file\":\"" + cfg.channelmap_file + "\",\"channels\":" +
                     std::to_string(cmap.size()) + ",\"in\":" +
                     std::to_string(cmap.inputs().size()) + ",\"out\":" +
                     std::to_string(cmap.outputs().size()));

    for (const Channel& ch : cmap.channels()) {
        const std::string& type = ch.dir == ChannelDir::In ? codec.state_type_name()
                                                            : codec.control_type_name();
        Topic* t = participant->create_topic(ch.topic, type, TOPIC_QOS_DEFAULT);
        if (ch.dir == ChannelDir::In) {
            DataReaderQos q = DATAREADER_QOS_DEFAULT;
            apply_qos_r(q);
            state_readers.push_back(sub->create_datareader(t, q, nullptr));
        } else {
            DataWriterQos q = DATAWRITER_QOS_DEFAULT;
            apply_qos_w(q);
            control_writers.push_back(pub->create_datawriter(t, q, nullptr));
        }
        VehicleTrace(run_id, id, "channel_bound",
                     "\"name\":\"" + ch.name + "\",\"dir\":\"" + ch.dir_name() + "\",\"topic\":\"" +
                         ch.topic + "\",\"type\":\"" + ch.type + "\"");
    }

    std::unique_ptr<IClockSource> clock =
        MakeClockSource(cfg.clock_mode, cfg.period_ms, &g_vehicle_stop, participant, run_id, id);
    VehicleTrace(run_id, id, "clock_start", "\"mode\":\"" + std::string(clock->mode()) + "\"");
    VehicleTrace(run_id, id, "behavior_start", "\"name\":\"" + std::string(behavior->name()) + "\"");
    VehicleTrace(run_id, id, "ready");

    EgoStateSample latest;   // last state seen from any in-channel (§9 feedback)
    bool have_state = false;
    TickInfo tick;
    uint64_t last_seq = 0;
    while (clock->WaitForNextTick(tick)) {
        for (DataReader* r : state_readers) {
            for (const EgoStateSample& s : codec.TakeState(r)) {
                latest = s;
                have_state = true;
            }
        }
        if (have_state) {
            std::ostringstream ex;
            ex << "\"tick_seq\":" << tick.seq << ",\"src_seq\":" << latest.seq
               << ",\"vx\":" << latest.vx << ",\"yaw\":" << latest.yaw
               << ",\"steering\":" << latest.steering_angle;
            VehicleTrace(run_id, id, "state_received", ex.str());
        }

        VehicleBehaviorContext bctx{tick.seq, tick.sim_time_ns, latest, have_state};
        VehicleControlSample c = behavior->OnTick(bctx);
        c.seq = tick.seq;                 // run loop is the single source of seq/stamp
        c.stamp_ns = tick.sim_time_ns;
        for (DataWriter* w : control_writers) {
            codec.PublishControl(w, c);
        }
        std::ostringstream ex;
        ex << "\"tick_seq\":" << tick.seq << ",\"behavior\":\"" << behavior->name()
           << "\",\"steering_angle\":" << c.steering_angle << ",\"acceleration\":" << c.acceleration
           << ",\"gear\":" << static_cast<int>(c.gear);
        VehicleTrace(run_id, id, "control_published", ex.str());

        clock->TickDone(tick);
        last_seq = tick.seq;
    }

    VehicleTrace(run_id, id, "run_stopping", "\"last_seq\":" + std::to_string(last_seq));
    participant->delete_contained_entities();
    clock.reset();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    VehicleTrace(run_id, id, "run_stopped", "\"last_seq\":" + std::to_string(last_seq));
    return 0;
}

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_RUN_HPP
