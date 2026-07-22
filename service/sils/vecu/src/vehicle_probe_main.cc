// sils_vecu_vehicle_probe -- MORAI/AWSIM plant-coupling free-run bring-up (Option 2 S1).
// docs/sils-morai-plant-adapter-design.md §5 S1.
//
// A SEPARATE entry point from the VISS RunProbe (run_loop.hpp): the VISS loop is a
// scalar publish-request model, this loop is the bus-coupled world model -- read canonical
// STATE (EgoState) off the bus, compute an actuation, publish canonical CONTROL
// (VehicleControl) back. Kept standalone so the VISS path is untouched; when a second
// consumer of this shape appears the common DDS/trace scaffolding can be lifted into a
// shared RunVehicleProbe (earned-abstraction), not before.
//
// S1 scope: wall clock (free-run, non-deterministic -- lockstep is S3), polling take each
// tick (no staging -- that's S2), a trivial longitudinal P-controller as the stand-in
// behavior (a real IVehicleBehavior port is deferred until a second controller exists).
// The point S1 proves is the BUS ROUND TRIP: EgoState in -> control out, over DDS, driven
// by the channel map. Types are the canonical contract (idl/dds/sils); a sim adapter
// (built elsewhere) is the anti-corruption layer that fills EgoState in SI.

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
#include "vehicle_wire.hpp"

namespace {

std::atomic<bool> g_stop{false};
void OnSignal(int) { g_stop.store(true); }

long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Minimal trace line, same JSONL shape as run_loop.hpp's TraceEvent (the conformance
// kit consumes this contract; docs/sils-understanding.md ch.6).
void Trace(const std::string& run_id, const std::string& instance_id, const std::string& event,
           const std::string& extra = "") {
    std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << instance_id
              << "\",\"event\":\"" << event << "\",\"wall_time_ms\":" << WallTimeMs()
              << (extra.empty() ? "" : ("," + extra)) << "}" << std::endl;
}

// S1 stand-in behavior: a trivial longitudinal P-controller. accel = Kp*(setpoint - vx);
// steering held 0; gear DRIVE. Deterministic given the state -- a real controller (and a
// proper IVehicleBehavior port) replaces this once a second consumer exists.
sils::VehicleControlSample ComputeControl(const sils::EgoStateSample& s, uint64_t seq,
                                          int64_t stamp_ns, double kp, double setpoint) {
    sils::VehicleControlSample c;
    c.seq = seq;
    c.stamp_ns = stamp_ns;
    c.steering_angle = 0.0;
    c.acceleration = kp * (setpoint - s.vx);
    c.gear = 3;  // DRIVE
    return c;
}

}  // namespace

int main() {
    using namespace eprosima::fastdds::dds;

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    const sils::ProbeConfig cfg = sils::ConfigFromEnv();
    const std::string& run_id = cfg.run_id;
    const std::string& id = cfg.instance_id;

    // This probe is channel-map driven -- fail-closed if none is configured (§14): running
    // a plant-coupling probe with no channels would silently do nothing.
    if (cfg.channelmap_file.empty()) {
        std::cerr << "vehicle_probe: SILS_CHANNELMAP is required (e.g. "
                     "config/sils/channelmap/ego-min.yaml)" << std::endl;
        return 2;
    }
    sils::ChannelMap cmap;
    std::string cmap_err;
    if (!cmap.Load(cfg.channelmap_file, cmap_err)) {
        std::cerr << "vehicle_probe: " << cmap_err << std::endl;
        return 2;
    }

    Trace(run_id, id, "run_start",
          "\"domain_id\":" + std::to_string(cfg.domain_id) + ",\"mode\":\"vehicle-coupling\"");

    DomainParticipantQos pqos;
    pqos.name("sils_vecu_vehicle_probe:" + id);
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant(
        cfg.domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "vehicle_probe: failed to create participant on domain " << cfg.domain_id
                  << std::endl;
        return 1;
    }

    sils::VehicleWireCodec codec(run_id);
    codec.RegisterTypes(participant);

    // QoS: RELIABLE + KEEP_LAST to match a typical adapter/SUT endpoint; TRANSIENT_LOCAL by
    // default for late-joiner robustness (opt out with SILS_DURABILITY=volatile), mirroring
    // run_loop.hpp's writer QoS.
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

    // Bind each channel to a reader (in / STATE) or writer (out / ACTUATION). S1 assumes
    // in-channels carry EgoState and out-channels carry VehicleControl (the ego-min map).
    std::vector<DataReader*> state_readers;
    std::vector<DataWriter*> control_writers;
    Trace(run_id, id, "channelmap_start",
          "\"file\":\"" + cfg.channelmap_file + "\",\"channels\":" + std::to_string(cmap.size()) +
              ",\"in\":" + std::to_string(cmap.inputs().size()) +
              ",\"out\":" + std::to_string(cmap.outputs().size()));

    for (const sils::Channel& ch : cmap.channels()) {
        const std::string& extra_type = ch.dir == sils::ChannelDir::In ? codec.state_type_name()
                                                                       : codec.control_type_name();
        Topic* t = participant->create_topic(ch.topic, extra_type, TOPIC_QOS_DEFAULT);
        if (ch.dir == sils::ChannelDir::In) {
            DataReaderQos q = DATAREADER_QOS_DEFAULT;
            apply_qos_r(q);
            state_readers.push_back(sub->create_datareader(t, q, nullptr));
        } else {
            DataWriterQos q = DATAWRITER_QOS_DEFAULT;
            apply_qos_w(q);
            control_writers.push_back(pub->create_datawriter(t, q, nullptr));
        }
        Trace(run_id, id, "channel_bound",
              "\"name\":\"" + ch.name + "\",\"dir\":\"" + ch.dir_name() + "\",\"topic\":\"" +
                  ch.topic + "\",\"type\":\"" + ch.type + "\"");
    }

    std::unique_ptr<sils::IClockSource> clock =
        sils::MakeClockSource(cfg.clock_mode, cfg.period_ms, &g_stop, participant, run_id, id);
    Trace(run_id, id, "clock_start", "\"mode\":\"" + std::string(clock->mode()) + "\"");
    Trace(run_id, id, "ready");

    sils::EgoStateSample latest;   // last state seen from any in-channel (§9 feedback)
    bool have_state = false;
    sils::TickInfo tick;
    uint64_t last_seq = 0;
    while (clock->WaitForNextTick(tick)) {
        // Read path: drain each state reader, keep the newest sample this tick.
        for (DataReader* r : state_readers) {
            for (const sils::EgoStateSample& s : codec.TakeState(r)) {
                latest = s;
                have_state = true;
            }
        }
        if (have_state) {
            std::ostringstream ex;
            ex << "\"tick_seq\":" << tick.seq << ",\"src_seq\":" << latest.seq
               << ",\"vx\":" << latest.vx << ",\"yaw\":" << latest.yaw
               << ",\"steering\":" << latest.steering_angle;
            Trace(run_id, id, "state_received", ex.str());
        }

        // Behavior + write path: compute one control from the latest state, publish to
        // every out-channel. Before any state arrives, latest is zero-initialized (safe
        // start: accel toward setpoint from v=0).
        const sils::VehicleControlSample c =
            ComputeControl(latest, tick.seq, tick.sim_time_ns, cfg.ctrl_kp, cfg.ctrl_setpoint);
        for (DataWriter* w : control_writers) {
            codec.PublishControl(w, c);
        }
        std::ostringstream ex;
        ex << "\"tick_seq\":" << tick.seq << ",\"steering_angle\":" << c.steering_angle
           << ",\"acceleration\":" << c.acceleration << ",\"gear\":" << static_cast<int>(c.gear);
        Trace(run_id, id, "control_published", ex.str());

        clock->TickDone(tick);
        last_seq = tick.seq;
    }

    Trace(run_id, id, "run_stopping", "\"last_seq\":" + std::to_string(last_seq));
    participant->delete_contained_entities();
    clock.reset();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    Trace(run_id, id, "run_stopped", "\"last_seq\":" + std::to_string(last_seq));
    return 0;
}
