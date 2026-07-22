// ChannelMap -- MORAI/AWSIM plant coupling, Option 2 (bus-coupled external world).
// See docs/sils-morai-plant-adapter-design.md (권장 Option 2) and
// docs/sils-morai-autoware-topology.html ①/④.
//
// In the MORAI/AWSIM topology the plant is NOT an in-process IPlant: it is a shared
// world on the DDS bus (authority=morai) that many consumers (N vECUs + Autoware) read
// at once. So a vECU does not `Step()` a local model -- it SUBSCRIBES to the sim's STATE
// signals as input and PUBLISHES its ACTUATION signals back. The closed loop is closed
// by the sim, over the bus. What a vECU needs, then, is not a scalar plant but a
// *declarative map* of the MIMO signals it cares about:
//
//   - IN  channels  (dir: in)  -- STATE the sim broadcasts, the vECU subscribes to.
//   - OUT channels  (dir: out) -- ACTUATION the vECU publishes back to the sim.
//
// Each channel names a signal by its VSS path (e.g. "Vehicle.Speed") so the vocabulary
// is the same one the in-repo VISS server (service/core/vehicle_info_mgr) already speaks
// -- a channel map is a subset/binding of that VSS namespace, not a new one (design doc
// §6 Q3: VSS-path direct binding).
//
// The map is loaded from a small YAML file (SILS_CHANNELMAP; config/sils/channelmap/*.yaml).
// The SDK is header-only and deliberately depends on nothing but Fast-DDS (CMakeLists.txt)
// -- exactly why json_min.hpp exists instead of a JSON library. So this parses a *narrow,
// fixed YAML subset* by hand (same spirit as ReplayPlant::Load / replay_source.hpp) rather
// than pulling in yaml-cpp. Anything outside the grammar is a fail-closed error with a
// file:line (§14): a mis-typed channel map must stop the run, not silently drop a signal.
//
// Grammar (2-space indent, one "key: value" per line, '#' starts a comment):
//
//   channels:
//     - name: Vehicle.Speed        # VSS path (required)
//       dir: in                    # in | out (required)
//       topic: morai/ego/speed     # DDS topic (required)
//       type: vss-scalar           # wire-type key the codec resolves (required)
//     - name: Vehicle.Chassis.Brake.PedalPosition
//       dir: out
//       topic: morai/ego/brake_cmd
//       type: vss-scalar
//
// S1 (free-run bring-up, design doc §5) uses this to load + validate + trace the map;
// binding each channel to a live DDS reader/writer is the following step once the
// VSS-signal DDS type is fixed. The type here is recorded and handed to the wire codec;
// it is intentionally not interpreted in this header.

#ifndef SILS_VECU_CHANNEL_MAP_HPP
#define SILS_VECU_CHANNEL_MAP_HPP

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace sils {

enum class ChannelDir { In, Out };  // In = subscribe (STATE); Out = publish (ACTUATION)

struct Channel {
    std::string name;   // VSS path, e.g. "Vehicle.Speed"
    ChannelDir dir = ChannelDir::In;
    std::string topic;  // DDS topic the signal rides on
    std::string type;   // wire-type key resolved by the codec (opaque here)

    const char* dir_name() const { return dir == ChannelDir::In ? "in" : "out"; }
};

class ChannelMap {
public:
    const std::vector<Channel>& channels() const { return channels_; }

    std::vector<const Channel*> inputs() const { return by_dir(ChannelDir::In); }
    std::vector<const Channel*> outputs() const { return by_dir(ChannelDir::Out); }

    std::size_t size() const { return channels_.size(); }
    bool empty() const { return channels_.empty(); }

    // Parse a channel-map YAML subset (grammar above). Returns false and sets `error`
    // (with a file:line) on the first thing it does not understand -- fail-closed so a
    // malformed map stops the run rather than silently coupling the wrong signals (§14).
    bool Load(const std::string& path, std::string& error) {
        channels_.clear();
        std::ifstream f(path);
        if (!f.is_open()) {
            error = "cannot open channel map: " + path;
            return false;
        }
        bool seen_header = false;   // saw the top-level "channels:" key
        bool have_cur = false;      // a channel is being accumulated
        bool cur_has_dir = false;   // its dir was set (dir has no safe default)
        Channel cur;
        std::string line;
        int lineno = 0;
        while (std::getline(f, line)) {
            ++lineno;
            const std::string content = Trim(StripComment(line));
            if (content.empty()) continue;  // blank / comment-only

            if (content == "channels:") {
                seen_header = true;
                continue;
            }
            if (!seen_header) {
                error = Where(path, lineno) + "expected 'channels:' before any entry";
                return false;
            }

            if (content.rfind("- ", 0) == 0) {
                // New list item: flush the previous channel, then parse the key on this line.
                if (!Flush(have_cur, cur_has_dir, cur, path, lineno, error)) return false;
                cur = Channel{};
                have_cur = true;
                cur_has_dir = false;
                if (!Apply(Trim(content.substr(2)), cur, cur_has_dir, path, lineno, error))
                    return false;
            } else if (content.find(':') != std::string::npos) {
                // Continuation field of the current channel.
                if (!have_cur) {
                    error = Where(path, lineno) + "field before any '- ' channel entry";
                    return false;
                }
                if (!Apply(content, cur, cur_has_dir, path, lineno, error)) return false;
            } else {
                error = Where(path, lineno) + "unrecognized line: '" + content + "'";
                return false;
            }
        }
        if (!Flush(have_cur, cur_has_dir, cur, path, lineno, error)) return false;
        if (channels_.empty()) {
            error = "channel map " + path + " defines no channels";
            return false;
        }
        return true;
    }

private:
    std::vector<Channel> channels_;

    std::vector<const Channel*> by_dir(ChannelDir d) const {
        std::vector<const Channel*> v;
        for (const Channel& c : channels_)
            if (c.dir == d) v.push_back(&c);
        return v;
    }

    static std::string StripComment(const std::string& s) { return s.substr(0, s.find('#')); }

    static std::string Trim(const std::string& s) {
        const std::size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return "";
        const std::size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    }

    static std::string Unquote(const std::string& s) {
        if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
            return s.substr(1, s.size() - 2);
        return s;
    }

    static std::string Where(const std::string& path, int lineno) {
        return "channel map " + path + ":" + std::to_string(lineno) + ": ";
    }

    // Parse one "key: value" into the current channel. Unknown keys / bad dir fail-close.
    static bool Apply(const std::string& kv, Channel& cur, bool& cur_has_dir,
                      const std::string& path, int lineno, std::string& error) {
        const std::size_t colon = kv.find(':');
        if (colon == std::string::npos) {
            error = Where(path, lineno) + "expected 'key: value', got '" + kv + "'";
            return false;
        }
        const std::string key = Trim(kv.substr(0, colon));
        const std::string val = Unquote(Trim(kv.substr(colon + 1)));
        if (key == "name") {
            cur.name = val;
        } else if (key == "topic") {
            cur.topic = val;
        } else if (key == "type") {
            cur.type = val;
        } else if (key == "dir") {
            if (val == "in") {
                cur.dir = ChannelDir::In;
            } else if (val == "out") {
                cur.dir = ChannelDir::Out;
            } else {
                error = Where(path, lineno) + "dir must be 'in' or 'out', got '" + val + "'";
                return false;
            }
            cur_has_dir = true;
        } else {
            error = Where(path, lineno) + "unknown key '" + key + "' (name|dir|topic|type)";
            return false;
        }
        if (val.empty() && key != "dir") {
            error = Where(path, lineno) + "empty value for '" + key + "'";
            return false;
        }
        return true;
    }

    // Commit the accumulated channel, validating required fields. `lineno` points at the
    // line that ended the entry (next '- ' or EOF), which is close enough to locate it.
    bool Flush(bool have_cur, bool cur_has_dir, const Channel& cur, const std::string& path,
               int lineno, std::string& error) {
        if (!have_cur) return true;
        if (cur.name.empty()) {
            error = Where(path, lineno) + "channel is missing 'name'";
            return false;
        }
        if (!cur_has_dir) {
            error = Where(path, lineno) + "channel '" + cur.name + "' is missing 'dir'";
            return false;
        }
        if (cur.topic.empty()) {
            error = Where(path, lineno) + "channel '" + cur.name + "' is missing 'topic'";
            return false;
        }
        if (cur.type.empty()) {
            error = Where(path, lineno) + "channel '" + cur.name + "' is missing 'type'";
            return false;
        }
        for (const Channel& c : channels_) {
            if (c.name == cur.name && c.dir == cur.dir) {
                error = Where(path, lineno) + "duplicate channel '" + cur.name + "' (dir " +
                        cur.dir_name() + ")";
                return false;
            }
        }
        channels_.push_back(cur);
        return true;
    }
};

}  // namespace sils

#endif  // SILS_VECU_CHANNEL_MAP_HPP
