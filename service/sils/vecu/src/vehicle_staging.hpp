// VehicleTickInbox -- typed per-tick input staging for the plant-coupling path
// (Option 2, bus-coupled MIMO). The TYPED counterpart of TickInbox (tick_inbox.hpp,
// architecture v3 Final §7.0/§7.3).
//
// TickInbox stages arrival COUNTS: the VISS scalar path only needs "how many inputs
// belong to this tick". A plant-coupling behavior instead consumes the state/target
// VALUE (feedback control), so this stages the actual typed sample each in-channel
// carries, alongside its reception sequence. Everything else is identical to TickInbox:
//
//   - OnSample(sample)  assigns the next reception sequence and stages the sample, in
//                       reception order (called from the DDS reader listener thread).
//   - FreezeTick()      at a tick boundary takes ALL currently-pending samples, in
//                       reception order, as this tick's input set and clears them;
//                       anything that arrives afterward defers to the next freeze
//                       (the "next_tick" late-sample policy).
//
// This yields the same §7.3 determinism invariant the tooling checks, per channel:
// concatenating each tick's frozen recv_seqs in tick order is strictly increasing
// (each sample consumed once, in reception order, partitioned across ticks). Arrival
// timing stays non-deterministic; consumption order does not (= local-staged). One inbox
// per in-channel, so each channel's stream is independently pinned and attributable.
//
// Pure logic (no DDS), like TickInbox, so it is trivially reasoned about and unit-testable;
// the run loop owns the DDS reader and calls OnSample from its listener.

#ifndef SILS_VECU_VEHICLE_STAGING_HPP
#define SILS_VECU_VEHICLE_STAGING_HPP

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace sils {

template <typename T>
class VehicleTickInbox {
public:
    struct Staged {
        uint64_t recv_seq;  // this channel's reception sequence (1-based, monotonic)
        T sample;           // the decoded typed sample (EgoStateSample / ControlTargetSample)
    };

    // Stage an arrival; returns its assigned reception sequence (§7.3).
    uint64_t OnSample(const T& sample) {
        std::lock_guard<std::mutex> lk(m_);
        uint64_t rs = ++recv_counter_;
        pending_.push_back(Staged{rs, sample});
        return rs;
    }

    // Freeze the current tick's input set: all pending arrivals, in reception order,
    // cleared from the queue (later arrivals belong to the next tick).
    std::vector<Staged> FreezeTick() {
        std::lock_guard<std::mutex> lk(m_);
        std::vector<Staged> frozen(pending_.begin(), pending_.end());
        pending_.clear();
        return frozen;
    }

    uint64_t received_count() {
        std::lock_guard<std::mutex> lk(m_);
        return recv_counter_;
    }

private:
    std::mutex m_;
    uint64_t recv_counter_ = 0;
    std::deque<Staged> pending_;
};

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_STAGING_HPP
