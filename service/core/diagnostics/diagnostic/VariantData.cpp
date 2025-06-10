#include "VariantData.hpp"

namespace diagnostic_manager {

void VariantData::SetName(const std::string& name) {
    name_ = name;
}

std::string VariantData::GetName() {
    return name_;
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

bool VariantData::GetString(const std::string& key, std::string& out) {
    if (!HasKey(key)) return false;
    auto& v = members_[key];
    return std::visit([&](auto&& val) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
            out = val;
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

} // namespace diagnostic_manager
