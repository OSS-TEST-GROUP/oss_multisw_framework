// SiLS Lockstep ClockMaster -- architecture v3 Final §8.2.
//
// Independent process (§8.2: Clock Master is a separate process by default) that
// drives lockstep time for a run: publishes TickGrant(seq, sim_time, dt) on
// sils/tick/grant, then waits for TickComplete from all participants on
// sils/tick/complete before granting the next tick. sim_time advances by a fixed
// dt each tick, giving the deterministic clock evaluate.py checks for.
//
// Phase 2 PoC simplification (allowed by §8.2 "PoC 편의"): participant membership
// is a fixed --participants N + per-tick timeout, not dynamic registration with
// required/optional classification. A tick that doesn't collect N distinct
// TickCompletes within --tick-timeout-ms is a hard abort (tick_timeout), which
// evaluate.py flags as a determinism break.
//
// Control plane shares the run's DDS domain (§12.1) -- VISS_DDS_DOMAIN_ID, same as
// the vECU probes. run_id from SILS_RUN_ID.

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <condition_variable>

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

#include "TickControlPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

namespace {

long long WallTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string GetEnvOr(const char* name, const std::string& fb) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : fb;
}

std::string g_run_id = "local";

void Trace(const std::string& event, const std::string& extra = "") {
    std::cout << "{\"run_id\":\"" << g_run_id << "\",\"event\":\"" << event
              << "\",\"wall_time_ms\":" << WallTimeMs()
              << (extra.empty() ? "" : ("," + extra)) << "}" << std::endl;
}

// Tracks matched grant subscribers so we don't start granting before the
// participants have discovered us (else early ticks are lost).
class GrantWriterListener : public DataWriterListener {
public:
    void on_publication_matched(DataWriter*, const PublicationMatchedStatus& s) override {
        matched.store(s.current_count);
    }
    std::atomic<int> matched{0};
};

// Collects TickCompletes as a map seq -> set(participant_id).
class CompleteListener : public DataReaderListener {
public:
    void on_data_available(DataReader* reader) override {
        TickComplete c;
        SampleInfo info;
        while (reader->take_next_sample(&c, &info) == RETCODE_OK) {
            if (!info.valid_data) continue;
            {
                std::lock_guard<std::mutex> l(m);
                done[c.tick_seq()].insert(c.participant_id());
            }
            cv.notify_all();
        }
    }
    // Wait until >= n distinct participants completed `seq`, or timeout. Returns count.
    int wait_for(uint64_t seq, int n, int timeout_ms) {
        std::unique_lock<std::mutex> l(m);
        cv.wait_for(l, std::chrono::milliseconds(timeout_ms),
                    [&] { return static_cast<int>(done[seq].size()) >= n; });
        return static_cast<int>(done[seq].size());
    }
    std::mutex m;
    std::condition_variable cv;
    std::map<uint64_t, std::set<std::string>> done;
};

int ArgInt(int argc, char** argv, const std::string& flag, int fb) {
    for (int i = 1; i < argc - 1; ++i) {
        if (flag == argv[i]) {
            try { return std::stoi(argv[i + 1]); } catch (...) { return fb; }
        }
    }
    return fb;
}

}  // namespace

int main(int argc, char** argv) {
    g_run_id = GetEnvOr("SILS_RUN_ID", "local");
    const int participants = ArgInt(argc, argv, "--participants", 1);
    const int ticks = ArgInt(argc, argv, "--ticks", 10);
    const int dt_ms = ArgInt(argc, argv, "--dt-ms", 10);
    // Minimum WALL time between grants. 0 = free-running (grant as fast as the
    // barrier closes) -- fine for pure vECU lockstep. Set it when a wall-clock
    // input source must land across ticks: a free-running clock races through all
    // ticks in ms, so wall-clock input arrives after the run and never stages
    // (§7.2). Wall-paced lockstep makes the run span the input arrival window.
    const int tick_interval_ms = ArgInt(argc, argv, "--tick-interval-ms", 0);
    const int tick_timeout_ms = ArgInt(argc, argv, "--tick-timeout-ms", 5000);
    const int startup_timeout_ms = ArgInt(argc, argv, "--startup-timeout-ms", 10000);
    const long long dt_ns = static_cast<long long>(dt_ms) * 1000000LL;

    int domain_id = 0;
    const std::string denv = GetEnvOr("VISS_DDS_DOMAIN_ID", "");
    if (!denv.empty()) {
        try { domain_id = std::stoi(denv); } catch (...) {}
    }

    DomainParticipantQos pqos;
    pqos.name("sils_clock_master:" + g_run_id);
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(
            domain_id, pqos, nullptr, StatusMask::none());
    if (!participant) {
        std::cerr << "clock_master: failed to create participant on domain " << domain_id << std::endl;
        return 2;
    }

    TypeSupport grant_type(new TickGrantPubSubType());
    grant_type.register_type(participant);
    TypeSupport complete_type(new TickCompletePubSubType());
    complete_type.register_type(participant);

    Publisher* pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    Topic* grant_topic = participant->create_topic("sils/tick/grant", grant_type.get_type_name(), TOPIC_QOS_DEFAULT);
    Topic* complete_topic = participant->create_topic("sils/tick/complete", complete_type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 256;
    GrantWriterListener wlistener;
    DataWriter* grant_writer = pub->create_datawriter(grant_topic, wqos, &wlistener);

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 256;
    CompleteListener clistener;
    DataReader* complete_reader = sub->create_datareader(complete_topic, rqos, &clistener);

    Trace("clock_start", "\"mode\":\"lockstep\",\"participants\":" + std::to_string(participants) +
          ",\"ticks\":" + std::to_string(ticks) + ",\"dt_ns\":" + std::to_string(dt_ns));

    // Wait for participants to subscribe to grants before we start (else early
    // ticks would be published to nobody).
    {
        int waited = 0;
        while (wlistener.matched.load() < participants && waited < startup_timeout_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waited += 50;
        }
        if (wlistener.matched.load() < participants) {
            Trace("clock_start_timeout", "\"matched\":" + std::to_string(wlistener.matched.load()) +
                  ",\"expected\":" + std::to_string(participants));
            participant->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant);
            return 1;
        }
    }

    int rc = 0;
    auto next_grant_at = std::chrono::steady_clock::now();
    for (int i = 1; i <= ticks; ++i) {
        // Wall-pacing: don't grant the next tick before its scheduled wall time.
        if (tick_interval_ms > 0) {
            std::this_thread::sleep_until(next_grant_at);
            next_grant_at += std::chrono::milliseconds(tick_interval_ms);
        }
        const uint64_t seq = static_cast<uint64_t>(i);
        const long long sim = static_cast<long long>(i) * dt_ns;

        TickGrant g;
        g.run_id(g_run_id);
        g.tick_seq(seq);
        g.sim_time_ns(sim);
        g.dt_ns(dt_ns);
        grant_writer->write(&g);
        Trace("tick_grant", "\"tick_seq\":" + std::to_string(seq) +
              ",\"sim_time_ns\":" + std::to_string(sim) + ",\"dt_ns\":" + std::to_string(dt_ns));

        const int got = clistener.wait_for(seq, participants, tick_timeout_ms);
        if (got < participants) {
            Trace("tick_timeout", "\"tick_seq\":" + std::to_string(seq) +
                  ",\"got\":" + std::to_string(got) + ",\"expected\":" + std::to_string(participants));
            rc = 1;
            break;
        }
        Trace("tick_barrier", "\"tick_seq\":" + std::to_string(seq) +
              ",\"completed\":" + std::to_string(got));
    }

    Trace("clock_stop", "\"exit_code\":" + std::to_string(rc));

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return rc;
}
