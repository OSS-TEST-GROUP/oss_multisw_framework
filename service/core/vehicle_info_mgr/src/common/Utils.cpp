#include "Utils.h"
#include "Logger.h"
#include <algorithm>
#include <typeinfo>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <charconv>
#include <vector>

std::string trim(const std::string& str) {
    const std::string whitespace = " \t";
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string UrlToPath(std::string url) {
    std::replace(url.begin(), url.end(), '/', '.');
    return url;
}

bool any_value_changed(const std::any& a, const std::any& b) {
    if (a.has_value() != b.has_value()) return true;
    if (!a.has_value()) return false;
    if (a.type() != b.type()) return true;

    const auto& type = a.type();

    if (type == typeid(std::string)) return std::any_cast<std::string>(a) != std::any_cast<std::string>(b);
    if (type == typeid(const char*)) return strcmp(std::any_cast<const char*>(a), std::any_cast<const char*>(b)) != 0;
    if (type == typeid(bool)) return std::any_cast<bool>(a) != std::any_cast<bool>(b);

    // Integer types
    if (type == typeid(uint8_t)) return std::any_cast<uint8_t>(a) != std::any_cast<uint8_t>(b);
    if (type == typeid(int8_t)) return std::any_cast<int8_t>(a) != std::any_cast<int8_t>(b);
    if (type == typeid(uint16_t)) return std::any_cast<uint16_t>(a) != std::any_cast<uint16_t>(b);
    if (type == typeid(int16_t)) return std::any_cast<int16_t>(a) != std::any_cast<int16_t>(b);
    if (type == typeid(uint32_t)) return std::any_cast<uint32_t>(a) != std::any_cast<uint32_t>(b);
    if (type == typeid(int32_t)) return std::any_cast<int32_t>(a) != std::any_cast<int32_t>(b);
    if (type == typeid(uint64_t)) return std::any_cast<uint64_t>(a) != std::any_cast<uint64_t>(b);
    if (type == typeid(int64_t)) return std::any_cast<int64_t>(a) != std::any_cast<int64_t>(b);

    // Floating point types
    if (type == typeid(float)) return std::any_cast<float>(a) != std::any_cast<float>(b);
    if (type == typeid(double)) return std::any_cast<double>(a) != std::any_cast<double>(b);

    if (type == typeid(std::shared_ptr<rapidjson::Document>)) {
        std::string str_a = any_to_string(a);
        std::string str_b = any_to_string(b);
        return str_a != str_b;
    }

    return false; // Or handle more complex types
}

std::string any_to_string(const std::any& val) {
    if (!val.has_value()) return "N/A";

    const auto& type = val.type();

    if (type == typeid(std::string)) return std::any_cast<std::string>(val);
    if (type == typeid(const char*)) return std::any_cast<const char*>(val);
    if (type == typeid(bool)) return std::any_cast<bool>(val) ? "true" : "false";
    
    // Integer types
    if (type == typeid(uint8_t)) return std::to_string(std::any_cast<uint8_t>(val));
    if (type == typeid(int8_t)) return std::to_string(std::any_cast<int8_t>(val));
    if (type == typeid(uint16_t)) return std::to_string(std::any_cast<uint16_t>(val));
    if (type == typeid(int16_t)) return std::to_string(std::any_cast<int16_t>(val));
    if (type == typeid(uint32_t)) return std::to_string(std::any_cast<uint32_t>(val));
    if (type == typeid(int32_t)) return std::to_string(std::any_cast<int32_t>(val));
    if (type == typeid(uint64_t)) return std::to_string(std::any_cast<uint64_t>(val));
    if (type == typeid(int64_t)) return std::to_string(std::any_cast<int64_t>(val));

    // Floating point types — std::to_chars yields the shortest decimal string
    // that round-trips back to the same value (e.g. 123.345f → "123.345"),
    // unlike std::to_string which pads to a fixed 6 fractional digits and
    // surfaces binary-representation noise ("123.345001").
    if (type == typeid(float)) {
        char buf[64];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), std::any_cast<float>(val));
        return std::string(buf, ptr);
    }
    if (type == typeid(double)) {
        char buf[64];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), std::any_cast<double>(val));
        return std::string(buf, ptr);
    }

    // For complex types (structs/arrays), serialize to JSON string
    if (type == typeid(std::map<std::string, std::any>) || type == typeid(std::vector<std::map<std::string, std::any>>)) {
        rapidjson::Document d;
        d.CopyFrom(any_to_json_value(val, d.GetAllocator()), d.GetAllocator());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);
        return buffer.GetString();
    }

    if (type == typeid(std::shared_ptr<rapidjson::Document>)) {
        const auto& doc_ptr = std::any_cast<std::shared_ptr<rapidjson::Document>>(val);
        if (doc_ptr) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc_ptr->Accept(writer);
            return buffer.GetString();
        }
        return "null";
    }

    return "Unsupported type: " + std::string(type.name());
}

std::string formatTimestamp(long long ms_since_epoch) {
    // Create a time_point from the milliseconds since epoch
    auto time_point = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms_since_epoch));
    auto in_time_t = std::chrono::system_clock::to_time_t(time_point);

    std::stringstream ss;
    // Use std::gmtime for UTC time and format it
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");

    // Append milliseconds
    auto ms = ms_since_epoch % 1000;
    ss << '.' << std::setfill('0') << std::setw(3) << ms << 'Z';

    return ss.str();
}

std::string getCurrentTimestamp() {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return formatTimestamp(now_ms);
}

rapidjson::Value any_to_json_value(const std::any& val, rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value v;

    if (!val.has_value()) {
        v.SetNull();
        LOG_ERROR << "No value found for type";
        return v;
    }

    const auto& type = val.type();

    if (type == typeid(std::string)) {
        v.SetString(std::any_cast<std::string>(val).c_str(), allocator);
    } else if (type == typeid(const char*)) {
        v.SetString(std::any_cast<const char*>(val), allocator);
    } else if (type == typeid(bool)) {
        v.SetBool(std::any_cast<bool>(val));
    } else if (type == typeid(uint8_t)) {
        v.SetUint(std::any_cast<uint8_t>(val));
    } else if (type == typeid(int8_t)) {
        v.SetInt(std::any_cast<int8_t>(val));
    } else if (type == typeid(uint16_t)) {
        v.SetUint(std::any_cast<uint16_t>(val));
    } else if (type == typeid(int16_t)) {
        v.SetInt(std::any_cast<int16_t>(val));
    } else if (type == typeid(uint32_t)) {
        v.SetUint(std::any_cast<uint32_t>(val));
    } else if (type == typeid(int32_t)) {
        v.SetInt(std::any_cast<int32_t>(val));
    } else if (type == typeid(uint64_t)) {
        v.SetUint64(std::any_cast<uint64_t>(val));
    } else if (type == typeid(int64_t)) {
        v.SetInt64(std::any_cast<int64_t>(val));
    } else if (type == typeid(float)) {
        v.SetFloat(std::any_cast<float>(val));
    } else if (type == typeid(double)) {
        v.SetDouble(std::any_cast<double>(val));
    } else if (type == typeid(std::shared_ptr<rapidjson::Document>)) {
        const auto& doc_ptr = std::any_cast<std::shared_ptr<rapidjson::Document>>(val);
        if (doc_ptr) {
            v.CopyFrom(*doc_ptr, allocator);
        } else {
            v.SetNull();
        }
    } else if (type == typeid(std::map<std::string, std::any>)) {
        v.SetObject();
        const auto& map = std::any_cast<const std::map<std::string, std::any>&>(val);
        for (const auto& [key, item_val] : map) {
            rapidjson::Value item_k(key.c_str(), allocator);
            rapidjson::Value item_v = any_to_json_value(item_val, allocator);
            v.AddMember(item_k, item_v, allocator);
        }
    } else if (val.type() == typeid(std::vector<std::map<std::string, std::any>>)) {
        v.SetArray();
        const auto& vec = std::any_cast<const std::vector<std::map<std::string, std::any>>&>(val);
        for (const auto& item_map : vec) {
            rapidjson::Value obj(rapidjson::kObjectType);
            for (const auto& [item_key, item_val] : item_map) {
                rapidjson::Value item_k(item_key.c_str(), allocator);
                rapidjson::Value item_v = any_to_json_value(item_val, allocator);
                obj.AddMember(item_k, item_v, allocator);
            }
            v.PushBack(obj, allocator);
        }
    } else if (val.type() == typeid(std::map<std::string, std::string>)) {
        v.SetObject();
        const auto& map = std::any_cast<const std::map<std::string, std::string>&>(val);
        for (const auto& [key, item_val] : map) {
            rapidjson::Value item_k(key.c_str(), allocator);
            rapidjson::Value item_v(item_val.c_str(), allocator);
            v.AddMember(item_k, item_v, allocator);
        }
    } else if (val.type() == typeid(std::vector<std::string>)) {
        v.SetArray();
        const auto& vec = std::any_cast<const std::vector<std::string>&>(val);
        for (const auto& str : vec) {
            rapidjson::Value s;
            s.SetString(str.c_str(), allocator);
            v.PushBack(s, allocator);
        }
    } else {
        LOG_ERROR << "Unsupported type: " << type.name();
    }
    // Note: Other vector types like vector<any> could be added here if needed.

    return v;
}

long long parseTimePeriod(const std::string& timePeriod) {
    long long totalMilliseconds = 0;
    if (timePeriod.empty() || timePeriod[0] != 'P') {
        return 0;
    }

    std::string currentNumber;
    
    size_t t_pos = timePeriod.find('T');

    // Date part
    if (t_pos != 1) { // if 'T' is present, it must not be at the start
        for (size_t i = 1; i < (t_pos == std::string::npos ? timePeriod.length() : t_pos); ++i) {
            char c = timePeriod[i];
            if (isdigit(c)) {
                currentNumber += c;
            } else if (c == 'D') {
                if (currentNumber.empty()) return 0; // D without number
                totalMilliseconds += std::stoll(currentNumber) * 24 * 60 * 60 * 1000;
                currentNumber.clear();
            } else {
                return 0; // Invalid character in date part
            }
        }
        if (!currentNumber.empty()) return 0; // Number without unit
    }


    if (t_pos == std::string::npos) {
         // If no 'T', it should only contain Date part, which is already parsed.
         // If there were other characters, it would have returned 0 already.
        return totalMilliseconds;
    }
    
    // Time part
    bool h_found = false, m_found = false, s_found = false;
    for (size_t i = t_pos + 1; i < timePeriod.length(); ++i) {
        char c = timePeriod[i];
        if (isdigit(c)) {
            currentNumber += c;
        } else if (c == 'H') {
            if (h_found || m_found || s_found || currentNumber.empty()) return 0;
            totalMilliseconds += std::stoll(currentNumber) * 3600000;
            currentNumber.clear();
            h_found = true;
        } else if (c == 'M') {
            if (m_found || s_found || currentNumber.empty()) return 0;
            totalMilliseconds += std::stoll(currentNumber) * 60000;
            currentNumber.clear();
            m_found = true;
        } else if (c == 'S') {
            if (s_found || currentNumber.empty()) return 0;
            totalMilliseconds += std::stoll(currentNumber) * 1000;
            currentNumber.clear();
            s_found = true;
        } else {
            return 0; // Invalid character
        }
    }
    if (!currentNumber.empty()) return 0; // Number without unit

    return totalMilliseconds;
}

std::string json_value_to_string(const std::string& datatype, const rapidjson::Value& value) {
    auto value_to_str = [](const rapidjson::Value& v) -> std::string {
        if (v.IsString()) return v.GetString();
        if (v.IsInt()) return std::to_string(v.GetInt());
        if (v.IsUint()) return std::to_string(v.GetUint());
        if (v.IsInt64()) return std::to_string(v.GetInt64());
        if (v.IsUint64()) return std::to_string(v.GetUint64());
        if (v.IsDouble()) return std::to_string(v.GetDouble());
        if (v.IsBool()) return v.GetBool() ? "true" : "false";
        return "";
    };

    bool is_array = (datatype.size() >= 2 && datatype.compare(datatype.size() - 2, 2, "[]") == 0);

    if (is_array && value.IsArray()) {
        std::string result = "[";
        for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + value_to_str(value[i]) + "\"";
        }
        result += "]";
        return result;
    }

    // Fallback: if it's an array but datatype isn't, or vice-versa, still try to return something sensible
    if (value.IsArray()) {
        std::string result = "[";
        for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
            if (i > 0) result += ",";
            result += value_to_str(value[i]);
        }
        result += "]";
        return result;
    }

    return value_to_str(value);
}

std::vector<std::string> parse_json_array_string(const std::string& json_array_str) {
    std::vector<std::string> result;
    std::string data = json_array_str;
    if (data.empty()) return result;

    // Handle potential single quotes by replacing them with double quotes (standard JSON)
    std::replace(data.begin(), data.end(), '\'', '\"');

    rapidjson::Document d;
    d.Parse(data.c_str());

    if (d.HasParseError() || !d.IsArray()) {
        return result;
    }

    for (const auto& item : d.GetArray()) {
        if (item.IsString()) {
            result.push_back(item.GetString());
        } else if (item.IsInt()) {
            result.push_back(std::to_string(item.GetInt()));
        } else if (item.IsDouble()) {
            result.push_back(std::to_string(item.GetDouble()));
        }
    }
    return result;
}
