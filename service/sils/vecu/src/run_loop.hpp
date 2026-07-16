#ifndef SILS_VECU_RUN_LOOP_HPP
#define SILS_VECU_RUN_LOOP_HPP

// sils::RunProbe -- the SiLS vECU tick loop as a reusable library entry point
// (libsils_vecu SDK, docs/sils-extension-model.md P1). It owns the whole run:
// DDS participant/writer setup on the resolved contract endpoint, the clock ->
// behavior -> (scenario fault) -> publish -> plant closed loop, the JSONL trace
// contract (docs/sils-understanding.md ch.6), and teardown.
//
// The two most-customized ports -- IBehavior (the control logic) and IPlant (the
// physical model) -- are INJECTED by the caller, so a developer's ECU main() builds
// its own and hands them in. The less-customized ports (IClockSource, IScenario) are
// built here from ProbeConfig so the common case needs no boilerplate. This is exactly
// the shape the default probe main (viss_probe_main.cc) and the extension-model
// example main.cc both use:
//     auto cfg = sils::ConfigFromEnv();
//     return sils::RunProbe(cfg, std::move(behavior), std::move(plant));
//
// Header-only: globals/helpers are `inline` so this compiles into whichever single
// executable includes it (each ECU is its own TU). Extracting RunProbe into a
// compiled library object is a later step; API stability (not ABI) is what the SDK
// contract needs (docs/sils-extension-model.md §2).

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "VissRequestPubSubTypes.hpp"
#include "behavior.hpp"
#include "clock_source.hpp"
#include "config.hpp"
#include "json_min.hpp"
#include "plant.hpp"
#include "replay_source.hpp"
#include "scenario.hpp"
#include "tick_inbox.hpp"
#include "wire.hpp"

namespace sils {

// Process-wide run state. `inline` so the definitions live in this header without an
// ODR clash: only the one executable that includes run_loop.hpp instantiates them.
inline std::atomic<bool> g_stop{false};
inline std::atomic<int> g_matched_count{0};

// vECU identity (Descriptor's instance_id, e.g. viss_probe.yaml's "viss-probe-1") --
// distinct from run_id (which identifies one Coordinator run, not one vECU). Only used
// for the participant name and sidecar trace metadata, never the DDS payload itself:
// the wire contract is whatever the real VISS server parses, and inventing extra
// payload fields would break that (architecture v3 Final §7.3).
inline std::string g_instance_id = "unknown";
// contract id: viss-request. Default /VIN/Vehicle; overridden from ProbeConfig::topic
// so a per-instance fleet (§9.5/§13) publishes to its resolved topic.
inline std::string g_topic = "/VIN/Vehicle";

inline void OnSignal(int /*signum*/) { g_stop.store(true); }

inline long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Trace line per architecture v3 Final §7.1/§12.2 (publish_intent/publish_result,
// run_id, wall time) -- deliberately minimal JSONL. This is the public trace contract
// the conformance kit (evaluate.py/parity.py) consumes; docs/sils-understanding.md ch.6.
inline void TraceEvent(const std::string& run_id, const std::string& event, int seq,
                       const std::string& extra_json_fields = "") {
    std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
              << "\",\"event\":\"" << event << "\",\"seq\":" << seq
              << ",\"wall_time_ms\":" << WallTimeMs() << ",\"topic\":\"" << g_topic << "\""
              << (extra_json_fields.empty() ? "" : ("," + extra_json_fields)) << "}"
              << std::endl;
}

// RequestId / EscapeJsonString / WrapRequestEnvelope now live in wire.hpp (the wire
// port owns payload encoding); RunProbe still uses RequestId for the trace contract.

class ProbeWriterListener : public eprosima::fastdds::dds::DataWriterListener {
public:
    explicit ProbeWriterListener(const std::string& run_id) : run_id_(run_id) {}

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override {
        g_matched_count.store(info.current_count);
        TraceEvent(run_id_, info.current_count_change > 0 ? "endpoint_matched" : "endpoint_unmatched",
                   0, "\"matched_count\":" + std::to_string(info.current_count));
    }

private:
    std::string run_id_;
};

// L1 (§11): receives correlated responses on the response topic and traces
// response_received{request_id, status, error_code}.
class ResponseListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    explicit ResponseListener(std::string run_id) : run_id_(std::move(run_id)) {}

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* /*reader*/,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override {
        matched_.store(info.current_count);
        if (info.current_count_change > 0) {
            TraceEvent(run_id_, "endpoint_matched", 0,
                       "\"role\":\"probe-response-reader\",\"matched_count\":" +
                           std::to_string(info.current_count));
        }
    }

    int matched() const { return matched_.load(); }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        VissRequestTopic sample;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            const std::string& p = sample.payload();
            const std::string rid = sils::JsonFindString(p, "requestId", "");
            const std::string status = sils::JsonFindString(p, "status", "");
            const std::string code = sils::JsonFindString(p, "errorCode", "");
            std::ostringstream extra;
            extra << "\"request_id\":\"" << rid << "\",\"status\":\"" << status << "\"";
            if (!code.empty()) extra << ",\"error_code\":\"" << code << "\"";
            TraceEvent(run_id_, "response_received", 0, extra.str());
        }
    }

private:
    std::string run_id_;
    std::atomic<int> matched_{0};
};

// Stages SUT input arrivals into a TickInbox (§7.3). Runs on the reader thread;
// assigns each arrival a reception sequence and traces input_received.
class InputStagingListener : public eprosima::fastdds::dds::DataReaderListener {
public:
    InputStagingListener(sils::TickInbox* inbox, std::string run_id, std::string topic)
        : inbox_(inbox), run_id_(std::move(run_id)), topic_(std::move(topic)) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        VissRequestTopic sample;
        eprosima::fastdds::dds::SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (!info.valid_data) continue;
            uint64_t rs = inbox_->OnSample();
            std::cout << "{\"run_id\":\"" << run_id_ << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"input_received\",\"recv_seq\":" << rs
                      << ",\"topic\":\"" << topic_ << "\"}" << std::endl;
        }
    }

private:
    sils::TickInbox* inbox_;
    std::string run_id_;
    std::string topic_;
};

inline std::string RecvSeqsJson(const std::vector<uint64_t>& seqs) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < seqs.size(); ++i) {
        if (i) os << ",";
        os << seqs[i];
    }
    os << "]";
    return os.str();
}

// Run one SiLS vECU probe to completion. `behavior` is required; `plant` may be null
// (open loop). Clock and scenario are built from `cfg`. Returns a process exit code
// (0 ok, 1 DDS setup failure, 2 config/scenario/replay fail-fast).
inline int RunProbe(const ProbeConfig& cfg, std::unique_ptr<IBehavior> behavior,
                    std::unique_ptr<IPlant> plant, std::unique_ptr<IWireCodec> wire = nullptr) {
    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    g_instance_id = cfg.instance_id;
    g_topic = cfg.topic;
    const std::string& run_id = cfg.run_id;

    // Wire port (§P4): what DDS type/payload goes on the wire. Injected by the caller, or
    // built from cfg (SILS_WIRE) -- default VISS envelope preserves the Phase 1 payload.
    if (!wire) wire = MakeWireCodec(cfg.wire_kind, cfg.response_topic_prefix);

    using namespace eprosima::fastdds::dds;

    TraceEvent(run_id, "run_start", 0, "\"domain_id\":" + std::to_string(cfg.domain_id));

    DomainParticipantQos pqos;
    // Suffixed with instance_id so multiple vECU instances show up as distinct,
    // greppable participants instead of colliding under one fixed name (§9.5).
    pqos.name("sils_vecu_viss_probe:" + g_instance_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            cfg.domain_id, pqos, nullptr, StatusMask::none());
    if (participant == nullptr) {
        std::cerr << "Failed to create DDS participant on domain " << cfg.domain_id << std::endl;
        return 1;
    }

    // The wire codec owns type registration; RunProbe only needs the registered name.
    wire->RegisterType(participant);
    const std::string wire_type_name = wire->type_name();
    TraceEvent(run_id, "wire_start", 0,
               "\"name\":\"" + std::string(wire->name()) + "\",\"type\":\"" + wire_type_name + "\"");

    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Topic* topic = participant->create_topic(g_topic, wire_type_name, TOPIC_QOS_DEFAULT);

    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    // Matches config/sils/qos/qos-profiles.yaml "viss-default" and the SUT's own reader
    // QoS -- a BEST_EFFORT writer would fail RxO matching against a RELIABLE reader.
    writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // Late-joiner robustness: offer TRANSIENT_LOCAL + KEEP_LAST so a verifier joining
    // while the probe is still alive gets the run's samples (superset of VOLATILE).
    // Opt out with SILS_DURABILITY=volatile (verifier must use the SAME setting).
    if (cfg.durability_transient_local) {
        writer_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        writer_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        writer_qos.history().depth = 256;
    }

    ProbeWriterListener listener(run_id);
    DataWriter* writer = publisher->create_datawriter(topic, writer_qos, &listener);
    if (writer == nullptr) {
        std::cerr << "Failed to create DataWriter for topic " << g_topic << std::endl;
        return 1;
    }
    // Report the negotiated request-writer QoS for L1 protocol conformance (§11):
    // analyze_protocol checks reliability == contracted RELIABLE.
    TraceEvent(run_id, "qos_negotiated", 0,
               "\"role\":\"probe-writer\",\"reliability\":\"RELIABLE\",\"durability\":\"" +
                   std::string(cfg.durability_name()) + "\"");

    // L1 protocol mode (§11): when SILS_PROTOCOL=reqresp, subscribe to the response
    // topic so a responder's correlated replies are received. Default oneway preserves
    // the fire-and-forget probe.
    const bool reqresp = cfg.reqresp;
    const std::string& response_topic = cfg.response_topic;
    ResponseListener response_listener(run_id);
    Topic* resp_topic_obj = nullptr;
    Subscriber* resp_sub = nullptr;
    if (reqresp) {
        resp_sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        resp_topic_obj = participant->create_topic(response_topic, wire_type_name,
                                                   TOPIC_QOS_DEFAULT);
        DataReaderQos rrq = DATAREADER_QOS_DEFAULT;
        rrq.reliability().kind = RELIABLE_RELIABILITY_QOS;
        if (cfg.durability_transient_local) {
            rrq.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            rrq.history().kind = KEEP_LAST_HISTORY_QOS;
            rrq.history().depth = 256;
        }
        resp_sub->create_datareader(resp_topic_obj, rrq, &response_listener);
        TraceEvent(run_id, "protocol_on", 0, "\"mode\":\"reqresp\",\"response_topic\":\"" +
                                                 response_topic + "\"");

        // First-response race (§11 L1): wait for the match, then settle so idle
        // heartbeats establish the reliable channel BEFORE the first request flows.
        const int match_timeout_ms = cfg.protocol_match_timeout_ms;
        const int settle_ms = cfg.protocol_settle_ms;
        int waited = 0;
        while (response_listener.matched() < 1 && waited < match_timeout_ms && !g_stop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            waited += 20;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
        TraceEvent(run_id, "protocol_ready", 0,
                   "\"response_matched\":" + std::to_string(response_listener.matched()) +
                       ",\"waited_ms\":" + std::to_string(waited + settle_ms));
    }

    TraceEvent(run_id, "ready", 0);

    // Clock port (§8): WallClock (default) or LockstepClock, selected by SILS_CLOCK_MODE.
    // The tick loop below is identical for both -- the mode lives behind IClockSource.
    std::unique_ptr<sils::IClockSource> clock = sils::MakeClockSource(
        cfg.clock_mode, cfg.period_ms, &g_stop, participant, run_id, g_instance_id);
    TraceEvent(run_id, "clock_start", 0, "\"mode\":\"" + std::string(clock->mode()) + "\"");

    // Behavior port (§11 L2): injected by the caller (default probe uses MakeBehavior).
    TraceEvent(run_id, "behavior_start", 0, "\"name\":\"" + std::string(behavior->name()) + "\"");

    // Scenario port (§11 negative path): tick-indexed fault injection from cfg.
    std::unique_ptr<sils::IScenario> scenario =
        sils::MakeScenario(cfg.scenario_kind, cfg.scenario_spec);
    // §14 fail-fast: a scheduled scenario with an empty/unparseable spec would silently
    // run as nominal and a "fault" run would falsely PASS. Reject before any tick.
    if (cfg.scenario_kind == "scheduled" && scenario->scheduled_faults() == 0) {
        std::cerr << "probe: SILS_SCENARIO=scheduled but SILS_SCENARIO_FAULTS injects no faults "
                     "(spec: '" << cfg.scenario_spec << "')" << std::endl;
        participant->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant);
        return 2;
    }
    TraceEvent(run_id, "scenario_start", 0,
               "\"name\":\"" + std::string(scenario->name()) +
                   "\",\"scheduled_faults\":" + std::to_string(scenario->scheduled_faults()));

    // Target attestation (§11 L4): declare which real target-like components this run
    // used so parity.py can gate on them. ARM64 alone is a COMPONENT of L4, not sufficient.
    TraceEvent(run_id, "target_attestation", 0,
               "\"arch\":\"" + cfg.attest_arch + "\",\"gateway\":\"" + cfg.attest_gateway +
                   "\",\"protocol\":\"" + cfg.attest_protocol + "\"");

    // Plant port (§11 L3): injected by the caller (default probe uses MakePlant); null
    // = open loop. The caller is responsible for reporting an unknown plant kind.
    if (plant) {
        plant->Reset(cfg.plant_init);
        std::ostringstream pr;
        pr << "\"model\":\"" << plant->name() << "\",\"mode\":\"" << plant->mode()
           << "\",\"initial\":" << cfg.plant_init;
        if (plant->fmi_version()) pr << ",\"fmi_version\":\"" << plant->fmi_version() << "\"";
        TraceEvent(run_id, "plant_reset", 0, pr.str());
    }

    // Run one tick: ask the behavior for this tick's output, trace it, apply the
    // scenario's fault, and hand it to the wire codec to encode + publish.
    auto publish_one = [&](const sils::BehaviorContext& ctx,
                           const sils::ScenarioAction& act) -> sils::BehaviorOutput {
        // The behavior runs unconditionally and deterministically; a fault only perturbs
        // what reaches the wire, never the vECU's internal computation.
        sils::BehaviorOutput out = behavior->OnTick(ctx);
        std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                  << "\",\"event\":\"behavior_output\",\"behavior\":\"" << behavior->name()
                  << "\",\"tick_seq\":" << ctx.tick_seq << ",\"value\":" << out.value << "}"
                  << std::endl;

        // §11 negative path: apply the scenario's fault for this tick.
        if (act.fault == sils::Fault::Drop) {
            TraceEvent(run_id, "scenario_fault", static_cast<int>(ctx.tick_seq),
                       "\"fault\":\"drop\",\"request_id\":\"" + RequestId(ctx.tick_seq) + "\"");
            return out;
        }

        const bool corrupt = act.fault == sils::Fault::Corrupt;
        const bool duplicate = act.fault == sils::Fault::Duplicate;
        TraceEvent(run_id, "publish_intent", static_cast<int>(ctx.tick_seq),
                   "\"matched_count\":" + std::to_string(g_matched_count.load()));
        // L1 (§11): emit request_sent so evaluate can correlate the responder's reply.
        if (reqresp) {
            TraceEvent(run_id, "request_sent", static_cast<int>(ctx.tick_seq),
                       "\"request_id\":\"" + RequestId(ctx.tick_seq) + "\",\"path\":\"" +
                           cfg.request_path + "\",\"action\":\"" + cfg.request_action + "\"" +
                           (corrupt ? ",\"corrupt\":true" : ""));
        }
        // The wire codec encodes out + writes on the wire, realizing the wire-level fault
        // (corrupt = one malformed sample; duplicate = two identical writes; else one).
        wire->Publish(writer, out, ctx.tick_seq,
                      corrupt ? sils::WireFault::Corrupt
                              : (duplicate ? sils::WireFault::Duplicate : sils::WireFault::Normal));
        if (duplicate) {
            // Same sample on the wire twice -> the SUT replies twice -> "duplicate".
            TraceEvent(run_id, "scenario_fault", static_cast<int>(ctx.tick_seq),
                       "\"fault\":\"duplicate\",\"request_id\":\"" + RequestId(ctx.tick_seq) + "\"");
        } else if (corrupt) {
            TraceEvent(run_id, "scenario_fault", static_cast<int>(ctx.tick_seq),
                       "\"fault\":\"corrupt\",\"request_id\":\"" + RequestId(ctx.tick_seq) + "\"");
        }
        TraceEvent(run_id, "publish_result", static_cast<int>(ctx.tick_seq));
        return out;
    };

    // Optional input staging (TickInbox, §7.3): freeze the pending arrivals per tick,
    // so which inputs a tick consumes is deterministic even though arrival timing isn't.
    const bool stage_input = cfg.stage_input;
    const std::string& input_topic = cfg.input_topic;
    const std::string& late_sample_policy = cfg.late_sample_policy;
    sils::TickInbox inbox;
    InputStagingListener input_listener(&inbox, run_id, input_topic);
    if (stage_input) {
        Subscriber* input_sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        Topic* input_topic_obj =
            participant->create_topic(input_topic, wire_type_name, TOPIC_QOS_DEFAULT);
        DataReaderQos irq = DATAREADER_QOS_DEFAULT;
        irq.reliability().kind = RELIABLE_RELIABILITY_QOS;
        irq.history().kind = KEEP_LAST_HISTORY_QOS;
        irq.history().depth = 256;
        input_sub->create_datareader(input_topic_obj, irq, &input_listener);
        TraceEvent(run_id, "input_staging_on", 0, "\"input_topic\":\"" + input_topic +
                                                      "\",\"late_sample_policy\":\"" +
                                                      late_sample_policy + "\"");
    }

    // Optional tick-indexed replay (§7.4): pre-recorded input count per tick. Takes
    // precedence over DDS staging when set.
    const std::string& replay_file = cfg.replay_file;
    const bool replay_mode = !replay_file.empty();
    sils::ReplaySource replay;
    if (replay_mode) {
        std::string err;
        if (!replay.Load(replay_file, err)) {
            std::cerr << "probe: " << err << std::endl;
            participant->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            return 2;
        }
        TraceEvent(run_id, "replay_start", 0,
                   "\"file\":\"" + replay_file + "\",\"inputs\":" + std::to_string(replay.total()));
    }

    sils::TickInfo tick;
    uint64_t last_seq = 0;
    uint64_t replay_recv = 0;  // synthetic reception sequence for replayed inputs
    while (clock->WaitForNextTick(tick)) {
        std::size_t staged_count = 0;
        if (replay_mode) {
            // §7.4: deliver this tick's pre-recorded inputs (source:"replay").
            std::size_t c = replay.CountForTick(tick.seq);
            std::vector<uint64_t> rs;
            for (std::size_t k = 0; k < c; ++k) {
                ++replay_recv;
                rs.push_back(replay_recv);
                std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                          << "\",\"event\":\"input_received\",\"recv_seq\":" << replay_recv
                          << ",\"source\":\"replay\"}" << std::endl;
            }
            staged_count = c;
            std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"tick_input_frozen\",\"tick_seq\":" << tick.seq
                      << ",\"recv_seqs\":" << RecvSeqsJson(rs) << ",\"count\":" << c
                      << ",\"source\":\"replay\"}" << std::endl;
        } else if (stage_input) {
            // §7.1: confirm this tick's inbound sample set at the tick boundary.
            std::vector<uint64_t> frozen = inbox.FreezeTick();
            staged_count = frozen.size();
            std::cout << "{\"run_id\":\"" << run_id << "\",\"instance_id\":\"" << g_instance_id
                      << "\",\"event\":\"tick_input_frozen\",\"tick_seq\":" << tick.seq
                      << ",\"recv_seqs\":" << RecvSeqsJson(frozen)
                      << ",\"count\":" << frozen.size() << "}" << std::endl;
        }
        // The behavior consumes this tick's staged input count (§7.3) + sim time, and
        // (L3) the plant output fed back from the previous tick (§9 closed loop).
        sils::BehaviorContext ctx{tick.seq, tick.sim_time_ns, staged_count};
        if (plant) ctx.plant_feedback = plant->output();
        sils::ScenarioAction act = scenario->OnTick({tick.seq, tick.sim_time_ns});
        sils::BehaviorOutput out = publish_one(ctx, act);
        if (plant) {
            // Close the loop: the command just published drives the plant one dt; the
            // new output becomes next tick's feedback.
            plant->SetInputs(out.value);
            double y = plant->Step(tick.dt_ns);
            std::ostringstream ps;
            ps << std::setprecision(12) << "\"tick_seq\":" << tick.seq
               << ",\"sim_time_ns\":" << tick.sim_time_ns << ",\"dt_ns\":" << tick.dt_ns
               << ",\"feedback_in\":" << ctx.plant_feedback << ",\"command\":" << out.value
               << ",\"output\":" << y;
            TraceEvent(run_id, "plant_step", static_cast<int>(tick.seq), ps.str());
        }
        clock->TickDone(tick);  // lockstep: sends TickComplete; wall: no-op
        last_seq = tick.seq;
    }

    // L1 (§11): responses arrive asynchronously; give in-flight replies time to land
    // before we tear the response reader down. Skipped in oneway mode.
    if (reqresp && !g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.response_drain_ms));
    }
    (void)resp_topic_obj;
    (void)resp_sub;  // owned by participant; freed below

    TraceEvent(run_id, "run_stopping", static_cast<int>(last_seq));

    // In lockstep the clock source created its own tick-control endpoints on this
    // participant, so tear ALL contained entities down in one shot before deleting the
    // participant (delete_participant fails if any contained entity survives).
    (void)writer;
    (void)publisher;
    (void)topic;  // owned by participant; freed below
    participant->delete_contained_entities();
    clock.reset();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    TraceEvent(run_id, "run_stopped", static_cast<int>(last_seq));
    return 0;
}

}  // namespace sils

#endif  // SILS_VECU_RUN_LOOP_HPP
