#ifndef SILS_VECU_VEHICLE_RUN_HPP
#define SILS_VECU_VEHICLE_RUN_HPP

// sils::RunVehicleProbe -- reusable tick loop for the MORAI/AWSIM plant-coupling path
// (Option 2, bus-coupled world). The bus-coupled counterpart of RunProbe (run_loop.hpp):
// it owns DDS participant/channel-map readers+writers, the free-run tick loop, the JSONL
// trace, and teardown, and calls an INJECTED IVehicleBehavior each tick (state in ->
// control out). A developer's plant-coupled ECU builds its behavior and hands it in --
// same shape as RunProbe taking IBehavior. docs/sils/sils-morai-plant-adapter-design.md §5.
//
// S1 scope: wall clock (free-run), polling take each tick, no lockstep (S3). S2 adds
// OPTIONAL per-tick input staging (SILS_STAGE_INPUT=1, §7.3): the in-channels then feed
// typed inboxes via listeners and each tick freezes its pinned input set, so which samples
// a tick consumes is deterministic even though arrival timing isn't. Off = S1 polling,
// preserved exactly. Header-only (inline) like run_loop.hpp so it compiles into the single
// executable that includes it.

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
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
#include "vehicle_input_fault.hpp"
#include "vehicle_staging.hpp"
#include "vehicle_wire.hpp"
#include "sils_participant_qos.hpp"

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
    // Locked (TraceMutex, clock_source.hpp): the DDS staging listeners emit input_received
    // from their own threads concurrently with the main loop's frozen/state/control lines,
    // and a bare `cout << ... << endl` is not atomic across threads -- interleaved writes
    // corrupt JSONL lines and surface as phantom staging violations downstream.
    std::lock_guard<std::mutex> lk(TraceMutex());
    std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << instance_id
              << "\",\"event\":\"" << event << "\",\"wall_time_ms\":" << VehicleWallMs()
              << (extra.empty() ? "" : ("," + extra)) << "}" << std::endl;
}

// Render a reception-sequence list as a JSON array for the tick_input_frozen trace (§7.3);
// the local counterpart of run_loop.hpp's RecvSeqsJson (kept separate so this header pulls
// in none of the VISS loop).
inline std::string VehicleRecvSeqsJson(const std::vector<uint64_t>& seqs) {
    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < seqs.size(); ++i) {
        if (i) os << ",";
        os << seqs[i];
    }
    os << "]";
    return os.str();
}

// Stages EgoState arrivals into a typed inbox (§7.3). Runs on the reader thread: decodes
// each arrival via the codec, assigns a reception sequence, and traces input_received. The
// typed, per-channel counterpart of run_loop.hpp's InputStagingListener (which counts only).
class StateStagingListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    StateStagingListener(VehicleWireCodec* codec, VehicleTickInbox<EgoStateSample>* inbox,
                         std::string run_id, std::string instance_id, std::string channel)
        : codec_(codec), inbox_(inbox), run_id_(std::move(run_id)),
          instance_id_(std::move(instance_id)), channel_(std::move(channel)) {}
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        for (const EgoStateSample& s : codec_->TakeState(reader)) {
            uint64_t rs = inbox_->OnSample(s);
            VehicleTrace(run_id_, instance_id_, "input_received",
                         "\"recv_seq\":" + std::to_string(rs) + ",\"channel\":\"" + channel_ +
                             "\",\"kind\":\"ego-state\",\"src_seq\":" + std::to_string(s.seq));
        }
    }

private:
    VehicleWireCodec* codec_;
    VehicleTickInbox<EgoStateSample>* inbox_;
    std::string run_id_, instance_id_, channel_;
};

// Stages ControlTarget arrivals (Profile B, Autoware target via the bridge) into a typed
// inbox. Same contract as StateStagingListener, different decode + kind.
class TargetStagingListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    TargetStagingListener(VehicleWireCodec* codec, VehicleTickInbox<ControlTargetSample>* inbox,
                          std::string run_id, std::string instance_id, std::string channel)
        : codec_(codec), inbox_(inbox), run_id_(std::move(run_id)),
          instance_id_(std::move(instance_id)), channel_(std::move(channel)) {}
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        for (const ControlTargetSample& t : codec_->TakeControlTarget(reader)) {
            uint64_t rs = inbox_->OnSample(t);
            VehicleTrace(run_id_, instance_id_, "input_received",
                         "\"recv_seq\":" + std::to_string(rs) + ",\"channel\":\"" + channel_ +
                             "\",\"kind\":\"control-target\",\"src_seq\":" + std::to_string(t.seq));
        }
    }

private:
    VehicleWireCodec* codec_;
    VehicleTickInbox<ControlTargetSample>* inbox_;
    std::string run_id_, instance_id_, channel_;
};

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
                 "\"domain_id\":" + std::to_string(cfg.domain_id) +
                     ",\"mode\":\"vehicle-coupling\",\"stage_input\":" +
                     (cfg.stage_input ? "true" : "false"));

    DomainParticipantQos pqos;
    sils::ApplySilsParticipantQos(pqos);   // see sils_participant_qos.hpp
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

    std::vector<DataReader*> state_readers;   // in, type canonical/ego-state (polled, non-staging)
    std::vector<DataReader*> target_readers;  // in, type canonical/control-target (Profile B)
    std::vector<DataWriter*> control_writers; // out, VehicleControl
    // in, type canonical/vehicle-control -- another vECU's OUTPUT read as our input
    // (arbitration). Names are kept alongside because the arbitration decision is logged
    // per channel; a bare vector of readers would make the trace unreadable.
    std::vector<DataReader*> request_readers;
    std::vector<std::string> request_names;

    // S2 staging (§7.3): typed per-tick inboxes + listeners, one per in-channel. Populated
    // only when cfg.stage_input -- the readers then feed listeners (arrival-time recv_seq)
    // instead of being polled in-tick. unique_ptr for stable addresses (the listener holds
    // &inbox, the reader holds &listener) and because VehicleTickInbox holds a mutex.
    const bool stage_input = cfg.stage_input;
    std::vector<std::unique_ptr<VehicleTickInbox<EgoStateSample>>> state_inboxes;
    std::vector<std::string> state_inbox_names;
    std::vector<std::unique_ptr<StateStagingListener>> state_listeners;
    std::vector<std::unique_ptr<VehicleTickInbox<ControlTargetSample>>> target_inboxes;
    std::vector<std::string> target_inbox_names;
    std::vector<std::unique_ptr<TargetStagingListener>> target_listeners;

    VehicleTrace(run_id, id, "channelmap_start",
                 "\"file\":\"" + cfg.channelmap_file + "\",\"channels\":" +
                     std::to_string(cmap.size()) + ",\"in\":" +
                     std::to_string(cmap.inputs().size()) + ",\"out\":" +
                     std::to_string(cmap.outputs().size()));

    for (const Channel& ch : cmap.channels()) {
        // Dispatch `in` channels by declared type: control-target (Autoware), vehicle-control
        // (another vECU's command -- arbitration), else ego-state (sim). `out` is always
        // VehicleControl.
        //
        // vehicle-control appears on BOTH sides: as `out` it is what we publish, as `in` it
        // is a peer's publication. The direction, not the type, decides which.
        const bool is_target = ch.type == "canonical/control-target";
        const bool is_request = ch.dir == ChannelDir::In && ch.type == "canonical/vehicle-control";
        const char* type = ch.dir != ChannelDir::In ? codec.control_type_name()
                           : is_target             ? codec.control_target_type_name()
                           : is_request            ? codec.control_type_name()
                                                   : codec.state_type_name();
        Topic* t = participant->create_topic(ch.topic, type, TOPIC_QOS_DEFAULT);
        if (ch.dir == ChannelDir::In) {
            DataReaderQos q = DATAREADER_QOS_DEFAULT;
            apply_qos_r(q);
            if (is_request) {
                // Deliberately NOT staged, even under SILS_STAGE_INPUT. Staging exists to pin
                // which arrival belongs to which tick for determinism of the ECU's own input;
                // an arbitration input is a peer's latest opinion and the rule (min/clamp) is
                // order-independent by construction, so freezing buys nothing and would add a
                // third inbox type to every staging report. Polled, newest-wins -- same as a
                // non-staged state channel.
                request_readers.push_back(sub->create_datareader(t, q, nullptr));
                request_names.push_back(ch.name);
            } else if (stage_input) {
                // Attach a staging listener so recv_seq is assigned at true arrival time;
                // the tick loop freezes the inbox at the boundary (no in-tick polling).
                if (is_target) {
                    target_inboxes.push_back(std::make_unique<VehicleTickInbox<ControlTargetSample>>());
                    target_inbox_names.push_back(ch.name);
                    target_listeners.push_back(std::make_unique<TargetStagingListener>(
                        &codec, target_inboxes.back().get(), run_id, id, ch.name));
                    sub->create_datareader(t, q, target_listeners.back().get());
                } else {
                    state_inboxes.push_back(std::make_unique<VehicleTickInbox<EgoStateSample>>());
                    state_inbox_names.push_back(ch.name);
                    state_listeners.push_back(std::make_unique<StateStagingListener>(
                        &codec, state_inboxes.back().get(), run_id, id, ch.name));
                    sub->create_datareader(t, q, state_listeners.back().get());
                }
            } else {
                DataReader* r = sub->create_datareader(t, q, nullptr);
                (is_target ? target_readers : state_readers).push_back(r);
            }
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

    // Inbound fault injection (§11 negative path, bus-coupled counterpart of IScenario):
    // perturb the ego STATE the behavior consumes on a deterministic tick schedule
    // (SILS_INPUT_FAULTS). Empty spec -> nominal (run byte-identical). Fail-fast (§14) if a
    // spec was given but parses to zero faults, so a typo can't silently disable the test.
    const std::string input_fault_spec = GetEnvOr("SILS_INPUT_FAULTS", "");
    std::unique_ptr<IInputFault> input_fault = MakeInputFault(input_fault_spec);
    if (!input_fault_spec.empty() && input_fault->scheduled_faults() == 0) {
        std::cerr << "RunVehicleProbe: SILS_INPUT_FAULTS set but no faults parsed from \""
                  << input_fault_spec << "\" (expected e.g. \"10:drop,20-25:nan,40:oor\")"
                  << std::endl;
        return 2;
    }
    if (input_fault->scheduled_faults() > 0) {
        VehicleTrace(run_id, id, "input_fault_on",
                     "\"schedule\":\"" + input_fault_spec + "\",\"faults\":" +
                         std::to_string(input_fault->scheduled_faults()));
    }
    if (stage_input) {
        VehicleTrace(run_id, id, "input_staging_on",
                     "\"late_sample_policy\":\"" + cfg.late_sample_policy +
                         "\",\"state_channels\":" + std::to_string(state_inboxes.size()) +
                         ",\"target_channels\":" + std::to_string(target_inboxes.size()));
    }
    VehicleTrace(run_id, id, "ready");

    EgoStateSample latest;   // last state seen from any in-channel (§9 feedback)
    bool have_state = false;
    ControlTargetSample latest_target;   // last Autoware target (Profile B)
    bool have_target = false;
    // Arbitration: one held value per request channel (ZOH across ticks with no arrival),
    // and the per-tick view handed to the behavior. Sized once -- indices match
    // request_readers/request_names.
    std::vector<VehicleControlSample> held_requests(request_readers.size());
    std::vector<char> have_held_request(request_readers.size(), 0);
    std::vector<ControlRequest> latest_requests;
    TickInfo tick;
    uint64_t last_seq = 0;
    while (clock->WaitForNextTick(tick)) {
        // Snapshot pre-tick state so a Drop fault (sensor dropout) can hold the last good
        // value across the outage instead of the freshly-arrived sample.
        const EgoStateSample prev_latest = latest;
        const bool prev_have_state = have_state;
        if (stage_input) {
            // §7.3: freeze each in-channel's pending arrivals as this tick's input set. The
            // newest frozen sample is the value the behavior sees (ZOH: held across ticks
            // with no arrival). tick_input_frozen records which recv_seqs were pinned here.
            for (std::size_t i = 0; i < state_inboxes.size(); ++i) {
                auto frozen = state_inboxes[i]->FreezeTick();
                std::vector<uint64_t> rs;
                rs.reserve(frozen.size());
                for (const auto& f : frozen) rs.push_back(f.recv_seq);
                VehicleTrace(run_id, id, "tick_input_frozen",
                             "\"tick_seq\":" + std::to_string(tick.seq) + ",\"channel\":\"" +
                                 state_inbox_names[i] + "\",\"kind\":\"ego-state\",\"recv_seqs\":" +
                                 VehicleRecvSeqsJson(rs) + ",\"count\":" + std::to_string(rs.size()));
                if (!frozen.empty()) {
                    latest = frozen.back().sample;
                    have_state = true;
                }
            }
            for (std::size_t i = 0; i < target_inboxes.size(); ++i) {
                auto frozen = target_inboxes[i]->FreezeTick();
                std::vector<uint64_t> rs;
                rs.reserve(frozen.size());
                for (const auto& f : frozen) rs.push_back(f.recv_seq);
                VehicleTrace(run_id, id, "tick_input_frozen",
                             "\"tick_seq\":" + std::to_string(tick.seq) + ",\"channel\":\"" +
                                 target_inbox_names[i] + "\",\"kind\":\"control-target\",\"recv_seqs\":" +
                                 VehicleRecvSeqsJson(rs) + ",\"count\":" + std::to_string(rs.size()));
                if (!frozen.empty()) {
                    latest_target = frozen.back().sample;
                    have_target = true;
                }
            }
        } else {
            for (DataReader* r : state_readers) {
                for (const EgoStateSample& s : codec.TakeState(r)) {
                    latest = s;
                    have_state = true;
                }
            }
            for (DataReader* r : target_readers) {
                for (const ControlTargetSample& t2 : codec.TakeControlTarget(r)) {
                    latest_target = t2;
                    have_target = true;
                }
            }
        }
        // Arbitration inputs: other vECUs' latest commands. Polled on both paths (see the
        // is_request note at bind time -- these are never staged). Newest per channel wins;
        // a channel that has said nothing yet simply does not appear this tick, which is the
        // honest representation: "no opinion" is not the same as "commands zero".
        latest_requests.clear();
        for (std::size_t i = 0; i < request_readers.size(); ++i) {
            bool got = false;
            VehicleControlSample newest{};
            for (const VehicleControlSample& v : codec.TakeControl(request_readers[i])) {
                newest = v;
                got = true;
            }
            if (got) {
                held_requests[i] = newest;
                have_held_request[i] = true;
            }
            // Hold the last value across ticks with no arrival (ZOH), same convention the
            // state channel uses. Without it a peer publishing slower than our tick would
            // flicker in and out of the arbitration set and the command would oscillate.
            if (have_held_request[i]) {
                latest_requests.push_back(ControlRequest{request_names[i], held_requests[i]});
            }
        }

        // Inbound fault injection (staging-layer interceptor): perturb the STATE the behavior
        // is about to consume. Applied here so both staging and free-run paths are covered,
        // and after acquisition so Drop can restore the pre-tick value (held across the outage).
        const InputFault fault_now = input_fault->OnTick(tick.seq);
        if (fault_now == InputFault::Drop) {
            latest = prev_latest;         // sensor dropout: hold last good state
            have_state = prev_have_state;
        } else if (fault_now != InputFault::None) {
            ApplyInputFault(fault_now, latest, have_state);  // Nan / OutOfRange: corrupt in place
        }
        if (fault_now != InputFault::None) {
            VehicleTrace(run_id, id, "input_fault",
                         "\"tick_seq\":" + std::to_string(tick.seq) +
                             ",\"channel\":\"Vehicle.EgoState\",\"kind\":\"ego-state\",\"mode\":\"" +
                             InputFaultName(fault_now) + "\"");
        }

        if (have_state) {
            // `vy` and `stamp_ns` carry no information for the behavior, but they are the
            // only two observables that answer questions the BUS cannot answer about the
            // sim adapter feeding us (tools/sils/ss2/analyze_wire.py):
            //   vy       -- is `velocity` in the VEHICLE frame or the MAP frame? Only vy
            //               distinguishes them, and only while yaw is changing: vehicle
            //               frame keeps vy ~= 0 through a curve, map frame does not.
            //   stamp_ns -- is the source stamping SIMULATION time or wall clock? A wall
            //               clock stamp is ~1.7e18 (ns since the Unix epoch); sim time
            //               restarts near 0 each run.
            // Both are additive: v1.json declares extra fields forward-compatible, so an
            // older vECU that omits them still validates.
            std::ostringstream ex;
            ex << "\"tick_seq\":" << tick.seq << ",\"src_seq\":" << latest.seq
               << ",\"stamp_ns\":" << latest.stamp_ns
               << ",\"vx\":" << latest.vx << ",\"vy\":" << latest.vy
               << ",\"yaw\":" << latest.yaw
               << ",\"steering\":" << latest.steering_angle;
            VehicleTrace(run_id, id, "state_received", ex.str());
        }
        if (have_target) {
            std::ostringstream ex;
            ex << "\"tick_seq\":" << tick.seq << ",\"src_seq\":" << latest_target.seq
               << ",\"steering\":" << latest_target.steering_angle
               << ",\"acceleration\":" << latest_target.acceleration
               << ",\"gear\":" << static_cast<int>(latest_target.gear);
            VehicleTrace(run_id, id, "target_received", ex.str());
        }

        VehicleBehaviorContext bctx{tick.seq,      tick.sim_time_ns, latest,
                                    have_state,    latest_target,    have_target,
                                    latest_requests};
        VehicleControlSample c = behavior->OnTick(bctx);
        // Arbitration decision, BEFORE the command line below. Counters alone cannot tell
        // "healthy" from "one peer has been winning every tick since second 3" -- and the
        // published command cannot either, because it looks like a normal command. The rule
        // lives in the behavior; the run loop only reports what the inputs were, which is
        // the part every arbitration rule has in common.
        if (!latest_requests.empty()) {
            std::ostringstream ax;
            ax << "\"tick_seq\":" << tick.seq << ",\"owner_acceleration\":"
               << (have_target ? latest_target.acceleration : c.acceleration)
               << ",\"chosen_acceleration\":" << c.acceleration << ",\"inputs\":{";
            for (std::size_t i = 0; i < latest_requests.size(); ++i) {
                if (i != 0) ax << ",";
                ax << "\"" << latest_requests[i].name << "\":"
                   << latest_requests[i].sample.acceleration;
            }
            ax << "}";
            VehicleTrace(run_id, id, "arbitrated", ax.str());
        }
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

    // Shutdown drain (§7.3 completeness): under 1-tick-lag staging, inputs that arrived
    // during the final tick(s) were never frozen by a subsequent tick -- a bounded run
    // would otherwise report them as unstaged/lost. Freeze the remainder here, in reception
    // order (the invariant holds), so a run with clock ticks == source sample count leaves
    // nothing unstaged. Marked drain=true: staged for accounting, no behavior ran on them.
    if (stage_input) {
        auto drain = [&](auto& inboxes, const std::vector<std::string>& names, const char* kind) {
            for (std::size_t i = 0; i < inboxes.size(); ++i) {
                auto frozen = inboxes[i]->FreezeTick();
                if (frozen.empty()) continue;
                std::vector<uint64_t> rs;
                rs.reserve(frozen.size());
                for (const auto& f : frozen) rs.push_back(f.recv_seq);
                VehicleTrace(run_id, id, "tick_input_frozen",
                             "\"tick_seq\":" + std::to_string(last_seq) + ",\"channel\":\"" +
                                 names[i] + "\",\"kind\":\"" + kind + "\",\"recv_seqs\":" +
                                 VehicleRecvSeqsJson(rs) + ",\"count\":" +
                                 std::to_string(rs.size()) + ",\"drain\":true");
            }
        };
        drain(state_inboxes, state_inbox_names, "ego-state");
        drain(target_inboxes, target_inbox_names, "control-target");
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
