#include "VariantData.hpp"
#include "logger/Log.hpp"

#include <algorithm>

namespace diagnostic_manager {

void VariantData::SetName(const std::string& name) {
    name_ = name;
}

std::string VariantData::GetName() {
    return name_;
}

VariantValue VariantData::GetValueAsVariant(const std::string& name) {
    VariantValue value;
    value.type = GetValueType(name);
    value.value = members_[name];
    return value;
}


VariantValue VariantData::GetValueAsVariantFromList(const std::string& name, uint64_t index) {
    VariantValue value;

    if (!HasKey(name)) return value;
    auto& v_list = members_[name];
    std::visit([&](auto&& val_list) -> void {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<char>>) {
            value.type = std::type_index(typeid(char));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<wchar_t>>) {
            value.type = std::type_index(typeid(wchar_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<int8_t>>) {
            value.type = std::type_index(typeid(int8_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<int16_t>>) {
            value.type = std::type_index(typeid(int16_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<int32_t>>) {
            value.type = std::type_index(typeid(int32_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
            value.type = std::type_index(typeid(int64_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            value.type = std::type_index(typeid(uint8_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<uint16_t>>) {
            value.type = std::type_index(typeid(uint16_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<uint32_t>>) {
            value.type = std::type_index(typeid(uint32_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<uint64_t>>) {
            value.type = std::type_index(typeid(uint64_t));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<float>>) {
            value.type = std::type_index(typeid(float));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<double>>) {
            value.type = std::type_index(typeid(double));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<bool>>) {
            value.type = std::type_index(typeid(bool));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            value.type = std::type_index(typeid(std::string));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else if constexpr (std::is_same_v<T, std::vector<std::wstring>>) {
            value.type = std::type_index(typeid(std::wstring));
            if (index < val_list.size()) { value.value = val_list[index]; }
        } else {
            LOG_ERROR("It's not an vector. name=" << name);
        }
    }, v_list);

    return value;
}


VariantData* VariantData::GetChild(const std::string& key) {
    if (!HasKey(key)) return nullptr;

    try {
        auto& variant = members_[key];
        if (std::holds_alternative<VariantData>(variant)) {
            return &(std::get<VariantData>(variant));
        }
    } catch (...) {}
    return nullptr;
}

bool VariantData::GetInt64(const std::string& key, int64_t& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_integral_v<T>) {
            out = static_cast<int64_t>(val);
            return true;
        }
        return false;
    }, v);
}

bool VariantData::GetUInt64(const std::string& key, uint64_t& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_integral_v<T>) {
            out = static_cast<uint64_t>(val);
            return true;
        }
        return false;
    }, v);
}

bool VariantData::GetDouble(const std::string& key, double& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_arithmetic_v<T>) {
            out = static_cast<double>(val);
            return true;
        }
        return false;
    }, v);
}

bool VariantData::GetBoolean(const std::string& key, bool& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) {
            out = static_cast<bool>(val);
            return true;
        }
        return false;
    }, v);
}

bool VariantData::GetString(const std::string& key, std::string& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
            out = val;
            return true;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::vector<char>>) {
            // Convert char array to string (handle null-terminated or full length)
            auto null_pos = std::find(val.begin(), val.end(), '\0');
            out = std::string(val.begin(), null_pos);
            return true;
        }
        return false;
    }, v);
}

VariantData* VariantData::GetChildFromList(const std::string& key, uint64_t idx) {
    if (!HasKey(key)) return nullptr;

    try {
        auto& variant_list = members_[key];
        if (std::holds_alternative<std::vector<VariantData>>(variant_list)) {
            return &(std::get<std::vector<VariantData>>(variant_list).at(idx));
        }
    } catch (...) {}
    return nullptr;
}


bool VariantData::GetInt64FromList(const std::string& key, uint64_t idx, int64_t& out) {
    if (!HasKey(key)) return false;
    auto& v_list = members_[key];
    return std::visit([&](auto&& val_list) -> bool {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<int8_t>> ||
                      std::is_same_v<T, std::vector<int16_t>> ||
                      std::is_same_v<T, std::vector<int32_t>> ||
                      std::is_same_v<T, std::vector<int64_t>> ||
                      std::is_same_v<T, std::vector<uint8_t>> ||
                      std::is_same_v<T, std::vector<uint16_t>> ||
                      std::is_same_v<T, std::vector<uint32_t>> ||
                      std::is_same_v<T, std::vector<uint64_t>>) {
            if (idx < val_list.size()) {
                out = static_cast<int64_t>(val_list[idx]);
                return true;
            }
        }
        return false;
    }, v_list);
}

bool VariantData::GetUInt64FromList(const std::string& key, uint64_t idx, uint64_t& out) {
    if (!HasKey(key)) return false;
    auto& v_list = members_[key];
    return std::visit([&](auto&& val_list) -> bool {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<int8_t>> ||
                      std::is_same_v<T, std::vector<int16_t>> ||
                      std::is_same_v<T, std::vector<int32_t>> ||
                      std::is_same_v<T, std::vector<int64_t>> ||
                      std::is_same_v<T, std::vector<uint8_t>> ||
                      std::is_same_v<T, std::vector<uint16_t>> ||
                      std::is_same_v<T, std::vector<uint32_t>> ||
                      std::is_same_v<T, std::vector<uint64_t>>) {
            if (idx < val_list.size()) {
                out = static_cast<uint64_t>(val_list[idx]);
                return true;
            }
        }
        return false;
    }, v_list);
}

bool VariantData::GetDoubleFromList(const std::string& key, uint64_t idx, double& out) {
    if (!HasKey(key)) return false;
    auto& v_list = members_[key];
    return std::visit([&](auto&& val_list) -> bool {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<float>> ||
                      std::is_same_v<T, std::vector<double>>) {
            if (idx < val_list.size()) {
                out = static_cast<double>(val_list[idx]);
                return true;
            }
        }
        return false;
    }, v_list);
}

bool VariantData::GetBooleanFromList(const std::string& key, uint64_t idx, bool& out) {
    if (!HasKey(key)) return false;
    auto& v_list = members_[key];
    return std::visit([&](auto&& val_list) -> bool {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<bool>>) {
            if (idx < val_list.size()) {
                out = static_cast<bool>(val_list[idx]);
                return true;
            }
        }
        return false;
    }, v_list);
}

bool VariantData::GetStringFromList(const std::string& key, uint64_t idx, std::string& out) {
    if (!HasKey(key)) return false;
    auto& v_list = members_[key];
    return std::visit([&](auto&& val_list) -> bool {
        using T = std::decay_t<decltype(val_list)>;
        if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            if (idx < val_list.size()) {
                out = static_cast<std::string>(val_list[idx]);
                return true;
            }
        }
        return false;
    }, v_list);
}

std::type_index VariantData::GetValueType(const std::string& name) {
    return std::visit([](auto&& val) {
        return std::type_index(typeid(val));
    }, members_[name]);
}

bool VariantData::HasKey(const std::string& name) {
    if (members_.count(name) > 0) {
        return true;
    } else {
        return false;
    }
}

std::vector<std::string> VariantData::GetKeyList() {
    std::vector<std::string> key_list;

    for (auto& iter : members_) {
        key_list.push_back(iter.first);
    }
    return key_list;
}

uint64_t VariantData::GetValueNum() {
    return members_.size();
}

VariantValue::VariantValue() : type(typeid(void)) {}
VariantValue::~VariantValue() {}

} // namespace diagnostic_manager
