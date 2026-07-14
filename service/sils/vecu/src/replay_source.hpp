// ReplaySource -- tick-indexed input replay (architecture v3 Final §7.4).
//
// Wall-clock input can't be reproduced run-to-run: which sample lands in which
// tick depends on arrival timing (§7.2). Replay removes that: a pre-recorded file
// fixes, per tick, how many inputs that tick delivers. The same file therefore
// yields the same tick->input attribution -- and hence the same behavior output --
// on every run, so a lockstep+staging+behavior run becomes a deterministic golden
// target (§7.4: "controlled inputs, no live feedback").
//
// File format (whitespace-separated, '#' comments, blank lines ignored):
//     # tick_seq  input_count
//     1 2
//     3 1
//     5 3
// Ticks not listed deliver 0 inputs. This is the minimal "replay bus source"
// (§7.2): a count per tick, enough to drive count-consuming behaviors (e.g.
// AccumulatorBehavior). Richer per-input payloads can extend the format later.
//
// Pure logic (no DDS/IO beyond loading the file), like TickInbox.

#ifndef SILS_VECU_REPLAY_SOURCE_HPP
#define SILS_VECU_REPLAY_SOURCE_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace sils {

class ReplaySource {
public:
    // Load the tick->count map. Returns false (with `error`) on open/parse failure.
    bool Load(const std::string& path, std::string& error) {
        std::ifstream f(path);
        if (!f.is_open()) { error = "cannot open replay file: " + path; return false; }
        std::string line;
        int lineno = 0;
        while (std::getline(f, line)) {
            ++lineno;
            std::string trimmed = line.substr(0, line.find('#'));
            std::istringstream ls(trimmed);
            uint64_t tick;
            long long count;
            if (!(ls >> tick)) continue;  // blank / comment-only
            if (!(ls >> count) || count < 0) {
                error = "replay " + path + ":" + std::to_string(lineno) +
                        ": expected 'tick_seq input_count' with count >= 0";
                return false;
            }
            counts_[tick] = static_cast<std::size_t>(count);
            total_ += static_cast<std::size_t>(count);
        }
        return true;
    }

    std::size_t CountForTick(uint64_t tick_seq) const {
        auto it = counts_.find(tick_seq);
        return it == counts_.end() ? 0 : it->second;
    }

    std::size_t total() const { return total_; }

private:
    std::map<uint64_t, std::size_t> counts_;
    std::size_t total_ = 0;
};

}  // namespace sils

#endif  // SILS_VECU_REPLAY_SOURCE_HPP
