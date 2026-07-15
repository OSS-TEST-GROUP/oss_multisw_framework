// json_min -- minimal JSON string-field extraction for SiLS payloads.
//
// SiLS wire payloads are small, controlled JSON built by our own code (VissRequest
// envelope + response). We need to read a couple of string fields back (requestId,
// status, topic, errorCode) without pulling in a JSON library. This does exactly
// that: find `"key"`, skip to its string value, return it (naive backslash-unescape).
// NOT a general JSON parser -- only for the fields our own encoders emit.

#ifndef SILS_VECU_JSON_MIN_HPP
#define SILS_VECU_JSON_MIN_HPP

#include <string>

namespace sils {

// Return the string value of the FIRST `"key":"value"` at the top level of `s`, or
// `fallback` if the key is absent or its value isn't a string. Escaped inner JSON
// (e.g. the envelope's "request" field, whose quotes are backslash-escaped) does not
// match `"key"` with real quotes, so a top-level key placed with real quotes wins.
inline std::string JsonFindString(const std::string& s, const std::string& key,
                                  const std::string& fallback = "") {
    const std::string pat = "\"" + key + "\"";
    std::size_t k = s.find(pat);
    if (k == std::string::npos) return fallback;
    std::size_t i = s.find(':', k + pat.size());
    if (i == std::string::npos) return fallback;
    ++i;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    if (i >= s.size() || s[i] != '"') return fallback;  // value is not a string
    ++i;
    std::string out;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;  // take the escaped char literally
        out += s[i++];
    }
    return out;
}

}  // namespace sils

#endif  // SILS_VECU_JSON_MIN_HPP
