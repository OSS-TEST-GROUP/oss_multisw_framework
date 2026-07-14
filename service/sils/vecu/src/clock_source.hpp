// IClockSource -- SiLS vECU clock port (architecture v3 Final §8, hexagonal port).
//
// The tick loop drives the vECU through ONE port with two adapters, so the probe
// body is clock-mode-agnostic:
//
//   while (clock->WaitForNextTick(tick)) { <do the tick's work>; clock->TickDone(tick); }
//
//   - WallClockSource   (§8.1 WallClock): free-running, period_ms cadence, no
//                        determinism -- the Phase 1 behavior, preserved exactly.
//   - LockstepClockSource (§8.2 LockstepClock): blocks until a ClockMaster
//                        TickGrant, replies TickComplete after the tick's work.
//
// This is the first port introduced (clock has two real implementations, so the
// abstraction is earned). IBus/IBehavior/IPlant/IScenario follow the same shape
// when a second implementation appears (plant at L3, behavior at L2, ...), not
// speculatively.
//
// LockstepClockSource creates its own tick-control endpoints on the SHARED run
// participant/domain (§12.1). TickControl types come from the generated headers.

#ifndef SILS_VECU_CLOCK_SOURCE_HPP
#define SILS_VECU_CLOCK_SOURCE_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "TickControlPubSubTypes.hpp"

namespace sils {

struct TickInfo {
    uint64_t seq = 0;
    int64_t sim_time_ns = 0;
    int64_t dt_ns = 0;
};

// Port.
class IClockSource {
public:
    virtual ~IClockSource() = default;
    // Block until the next tick is available. Returns false when the run should end
    // (stop signal, or clock master finished / went away). Fills `out` on true.
    virtual bool WaitForNextTick(TickInfo& out) = 0;
    // The tick's work is done: lockstep replies TickComplete; wall is a no-op.
    virtual void TickDone(const TickInfo& tick) = 0;
    virtual const char* mode() const = 0;
};

// --- WallClock adapter (§8.1) -----------------------------------------------
// Preserves Phase 1 behavior: publish immediately, then a period_ms gap between
// ticks, checking the stop flag in small slices. sim_time is a nominal
// period-derived value (wall runs are non-deterministic by construction).
class WallClockSource : public IClockSource {
public:
    WallClockSource(int period_ms, std::atomic<bool>* stop)
        : period_ms_(period_ms), stop_(stop) {}

    bool WaitForNextTick(TickInfo& out) override {
        if (stop_->load()) return false;
        if (seq_ > 0) {  // gap between ticks (not before the first)
            for (int w = 0; w < period_ms_ && !stop_->load(); w += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(std::min(50, period_ms_ - w)));
            }
            if (stop_->load()) return false;
        }
        ++seq_;
        out.seq = seq_;
        out.dt_ns = static_cast<int64_t>(period_ms_) * 1000000LL;
        out.sim_time_ns = static_cast<int64_t>(seq_) * out.dt_ns;
        return true;
    }
    void TickDone(const TickInfo&) override {}
    const char* mode() const override { return "wall"; }

private:
    int period_ms_;
    std::atomic<bool>* stop_;
    uint64_t seq_ = 0;
};

// --- Lockstep adapter (§8.2) ------------------------------------------------
class LockstepClockSource : public IClockSource {
public:
    LockstepClockSource(eprosima::fastdds::dds::DomainParticipant* participant,
                        std::string run_id, std::string participant_id,
                        std::atomic<bool>* stop)
        : run_id_(std::move(run_id)),
          participant_id_(std::move(participant_id)),
          stop_(stop),
          participant_(participant),
          listener_(&queue_) {
        using namespace eprosima::fastdds::dds;

        grant_type_ = TypeSupport(new TickGrantPubSubType());
        grant_type_.register_type(participant_);
        complete_type_ = TypeSupport(new TickCompletePubSubType());
        complete_type_.register_type(participant_);

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        grant_topic_ = participant_->create_topic(
            "sils/tick/grant", grant_type_.get_type_name(), TOPIC_QOS_DEFAULT);
        complete_topic_ = participant_->create_topic(
            "sils/tick/complete", complete_type_.get_type_name(), TOPIC_QOS_DEFAULT);

        // Control plane is RELIABLE + KEEP_LAST(deep) so no grant is silently dropped.
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.history().kind = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth = 256;
        grant_reader_ = subscriber_->create_datareader(grant_topic_, rqos, &listener_);

        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        complete_writer_ = publisher_->create_datawriter(complete_topic_, wqos, nullptr);
    }

    bool WaitForNextTick(TickInfo& out) override {
        return queue_.pop(out, stop_);
    }

    void TickDone(const TickInfo& tick) override {
        TickComplete msg;
        msg.run_id(run_id_);
        msg.tick_seq(tick.seq);
        msg.participant_id(participant_id_);
        msg.status(0);
        complete_writer_->write(&msg);
        // Trace line matches evaluate.py's lockstep contract (§8.2): the adapter
        // owns emission of tick_complete since it knows when the ack is sent.
        std::cout << "{\"run_id\":\"" << run_id_ << "\",\"instance_id\":\""
                  << participant_id_ << "\",\"event\":\"tick_complete\",\"tick_seq\":"
                  << tick.seq << ",\"sim_time_ns\":" << tick.sim_time_ns << "}" << std::endl;
    }

    const char* mode() const override { return "lockstep"; }

private:
    // Thread-safe grant queue; pop() wakes periodically to re-check the stop flag
    // so a stop unblocks the tick loop without needing to signal from a handler.
    class GrantQueue {
    public:
        void push(const TickInfo& t) {
            { std::lock_guard<std::mutex> l(m_); q_.push(t); }
            cv_.notify_one();
        }
        bool pop(TickInfo& out, std::atomic<bool>* stop) {
            std::unique_lock<std::mutex> l(m_);
            while (true) {
                if (!q_.empty()) { out = q_.front(); q_.pop(); return true; }
                if (stop->load()) return false;
                cv_.wait_for(l, std::chrono::milliseconds(100));
            }
        }
    private:
        std::mutex m_;
        std::condition_variable cv_;
        std::queue<TickInfo> q_;
    };

    class GrantListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        explicit GrantListener(GrantQueue* q) : q_(q) {}
        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
            TickGrant g;
            eprosima::fastdds::dds::SampleInfo info;
            while (reader->take_next_sample(&g, &info) == eprosima::fastdds::dds::RETCODE_OK) {
                if (info.valid_data) {
                    TickInfo t;
                    t.seq = g.tick_seq();
                    t.sim_time_ns = g.sim_time_ns();
                    t.dt_ns = g.dt_ns();
                    q_->push(t);
                }
            }
        }
    private:
        GrantQueue* q_;
    };

    std::string run_id_;
    std::string participant_id_;
    std::atomic<bool>* stop_;
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::TypeSupport grant_type_;
    eprosima::fastdds::dds::TypeSupport complete_type_;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* grant_topic_ = nullptr;
    eprosima::fastdds::dds::Topic* complete_topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* grant_reader_ = nullptr;
    eprosima::fastdds::dds::DataWriter* complete_writer_ = nullptr;
    GrantQueue queue_;
    GrantListener listener_;
};

// Factory: SILS_CLOCK_MODE selects the adapter. participant is only needed for
// lockstep (control-plane endpoints); wall ignores it.
inline std::unique_ptr<IClockSource> MakeClockSource(
    const std::string& mode, int period_ms, std::atomic<bool>* stop,
    eprosima::fastdds::dds::DomainParticipant* participant,
    const std::string& run_id, const std::string& participant_id) {
    if (mode == "lockstep") {
        return std::make_unique<LockstepClockSource>(participant, run_id, participant_id, stop);
    }
    return std::make_unique<WallClockSource>(period_ms, stop);
}

}  // namespace sils

#endif  // SILS_VECU_CLOCK_SOURCE_HPP
