// TickInbox -- consumer-local input staging (architecture v3 Final §7.0/§7.3).
//
// A vECU that consumes SUT input can't make the whole SiLS deterministic just by
// running its own lockstep loop (§7.2): DDS samples arrive on network/scheduler
// timing, so WHICH already-arrived samples belong to WHICH tick must be pinned.
// TickInbox does exactly that and nothing more (§7.0 "does not control arrival
// time, only which received samples the current tick consumes"):
//
//   - OnSample()  assigns the next reception sequence to an arrival and stages it
//                 (called from the DDS reader listener thread).
//   - FreezeTick() at a tick boundary takes ALL currently-pending inputs, in
//                 reception order, as this tick's input set and clears them;
//                 anything that arrives afterward defers to the next freeze.
//
// By construction this yields the §7.3 determinism invariant the tooling checks:
// concatenating each tick's frozen recv_seqs in tick order is strictly increasing
// (inputs consumed once, in reception order, partitioned across ticks). Arrival
// timing stays non-deterministic; consumption order does not (= local-staged).
//
// Pure logic (no DDS) so it is trivially reasoned about and reusable; the probe
// owns the DDS reader and calls OnSample from its listener.

#ifndef SILS_VECU_TICK_INBOX_HPP
#define SILS_VECU_TICK_INBOX_HPP

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace sils {

class TickInbox {
public:
    // Stage an arrival; returns its assigned reception sequence (§7.3).
    uint64_t OnSample() {
        std::lock_guard<std::mutex> lk(m_);
        uint64_t rs = ++recv_counter_;
        pending_.push_back(rs);
        return rs;
    }

    // Freeze the current tick's input set: all pending arrivals, in reception
    // order, cleared from the queue (later arrivals belong to the next tick).
    std::vector<uint64_t> FreezeTick() {
        std::lock_guard<std::mutex> lk(m_);
        std::vector<uint64_t> frozen(pending_.begin(), pending_.end());
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
    std::deque<uint64_t> pending_;
};

}  // namespace sils

#endif  // SILS_VECU_TICK_INBOX_HPP
