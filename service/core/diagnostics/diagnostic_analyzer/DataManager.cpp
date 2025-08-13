#include "DataManager.hpp"

#include <vector>
#include <stdexcept>
#include <cctype>
#include <type_traits>

#include "logger/Log.hpp"

namespace diagnostic_manager {

bool DataManager::UpdateData(std::shared_ptr<VariantData> variant_data) {
    std::string name = variant_data->GetName();
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(data_handling_lock_);
    data_list_[name] = variant_data;
    updated_data_list_.insert(variant_data);
    return true;
}

std::shared_ptr<VariantData> DataManager::GetData(const std::string& name) {
    auto it = data_list_.find(name);
    if (it == data_list_.end()) {
        return nullptr;
    }
    return it->second;
}

std::set<std::shared_ptr<VariantData>> DataManager::GetAllUpdatedData() {
    std::set<std::shared_ptr<VariantData>> result;
    {
        std::lock_guard<std::mutex> lock(data_handling_lock_);
        result.insert(updated_data_list_.begin(), updated_data_list_.end());
        updated_data_list_.clear();
    }
    return result;
}

template<typename T>
bool DataManager::FetchValue(const std::string& path, T& out) {
    // Ensure T is one of the supported types.
    static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, bool> ||
                  std::is_same_v<T, double> || std::is_same_v<T, std::string>,
                  "FetchValue supports only int64_t, uint64_t, double, or std::string.");

    if (path.empty()) {
        return false;
    }

    // Split the input path into tokens separated by '.'.
    std::vector<std::string> tokens = GetPathTokens(path);

    std::lock_guard<std::mutex> lock(data_handling_lock_);

    std::shared_ptr<VariantData> root = GetData(tokens[0]);
    VariantData* current = FindChildByExploring(root, tokens);
    if (!current) {
        return false;
    }

    // Parse the current token for a key (name part) and optional indices.
    std::string key_name;
    std::vector<size_t> indices;
    if (!GetIndiciesInfo(tokens.back(), key_name, indices)) {
        LOG_ERROR("FetchValue: Failed to get indices.");
        return false;
    }

    if (indices.empty()) {
        if constexpr(std::is_same_v<T, int64_t>) {
            return current->GetInt64(key_name, out);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return current->GetUInt64(key_name, out);
        } else if constexpr(std::is_same_v<T, double>) {
            return current->GetDouble(key_name, out);
        } else if constexpr(std::is_same_v<T, bool>) {
            return current->GetBoolean(key_name, out);
        } else if constexpr(std::is_same_v<T, std::string>) {
            return current->GetString(key_name, out);
        } else {
            LOG_ERROR("No Output. not supported type.");
        }
    } else {
        if constexpr(std::is_same_v<T, int64_t>) {
            return current->GetInt64FromList(key_name, indices[0], out);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return current->GetUInt64FromList(key_name, indices[0], out);
        } else if constexpr(std::is_same_v<T, double>) {
            return current->GetDoubleFromList(key_name, indices[0], out);
        } else if constexpr(std::is_same_v<T, bool>) {
            return current->GetBooleanFromList(key_name, indices[0], out);
        } else if constexpr(std::is_same_v<T, std::string>) {
            return current->GetStringFromList(key_name, indices[0], out);
        } else {
            LOG_ERROR("No Output. not supported type.");
        }
    }

    return false;
}

bool DataManager::FetchValueAsVariant(const std::string& path, VariantValue& value) {
    if (path.empty()) {
        return false;
    }

    // Split the input path into tokens separated by '.'
    std::vector<std::string> tokens = GetPathTokens(path);

    std::lock_guard<std::mutex> lock(data_handling_lock_);

    std::shared_ptr<VariantData> root = GetData(tokens[0]);
    VariantData* current = FindChildByExploring(root, tokens);
    if (!current) {
        return false;
    }

    // Parse the current token for a key (name part) and optional indices.
    std::string key_name;
    std::vector<size_t> indices;
    if (!GetIndiciesInfo(tokens.back(), key_name, indices)) {
        LOG_ERROR("FetchValue: Failed to get indices.");
        return false;
    }

    if (indices.empty()) {
        value = current->GetValueAsVariant(key_name);
    } else {
        value = current->GetValueAsVariantFromList(key_name, indices[0]);
    }
    return value.type != std::type_index(typeid(void));
}

std::vector<std::string> DataManager::GetPathTokens(const std::string& path) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < path.size()) {
        size_t dotPos = path.find('.', start);
        if (dotPos == std::string::npos) {
            tokens.push_back(path.substr(start));
            break;
        } else {
            tokens.push_back(path.substr(start, dotPos - start));
            start = dotPos + 1;
        }
    }
    return tokens;
}

bool DataManager::GetIndiciesInfo(const std::string& token, std::string& key, std::vector<size_t>& indices) {
    uint64_t i = 0;

    if (std::isalpha(static_cast<unsigned char>(token[0])) || token[0] == '_') {
        while (i < token.size() && token[i] != '[') {
            if (token[i] == ']') {
                LOG_ERROR("GetIndiciesInfo: ']' without matching '[' in path. token=" << token);
                return false;
            }
            key = token.substr(0, i+1);
            ++i;
        }
    }

    while (i < token.size()) {
        if (token[i] == '[') {
            ++i;
            if (i >= token.size() || !std::isdigit(static_cast<unsigned char>(token[i]))) {
                LOG_ERROR("FetchValue: Invalid index in path token = " << token);
                return false;
            }
            size_t index = 0;
            while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) {
                index = index * 10 + (token[i] - '0');
                ++i;
            }
            if (i >= token.size() || token[i] != ']') {
                LOG_ERROR("FetchValue: Missing ']' in path token = " << token);
                return false;
            }
            indices.push_back(index);
            ++i; // move past ']'
        } else {
            // Unexpected character in token (should only be '[' or ']' at this point).
            LOG_ERROR("FetchValue: Unexpected character '" << std::string(1, token[i]) << "' in path token = " << token);
            return false;
        }
    }

    return true;
}

VariantData* DataManager::FindChildByExploring(std::shared_ptr<VariantData> root, std::vector<std::string> tokens) {
    VariantData* current = root.get();
    for (size_t i = 1; i < tokens.size() - 1; ++i) {
        const std::string& token = tokens[i];
        if (token.empty()) {
            LOG_ERROR("FetchValue: Invalid empty token in path");
            return nullptr;
        }

        // Parse the current token for a key (name part) and optional indices.
        std::string key_name;
        std::vector<size_t> indices;
        if (!GetIndiciesInfo(token, key_name, indices)) {
            LOG_ERROR("FetchValue: Failed to get indices.");
        }

        if (i < tokens.size() - 1) {
        // At the middle of tokens. Exploring in a tree of the VariantData.
            VariantData* next = nullptr;
            if (indices.empty()) {
                next = current->GetChild(key_name);
            } else {
                next = current->GetChildFromList(key_name, indices[0]);
            }

            if (!next) {
                return nullptr;
            }
            current = next;
        }
    }

    return current;
}

template bool DataManager::FetchValue<int64_t>(const std::string&, int64_t&);
template bool DataManager::FetchValue<uint64_t>(const std::string&, uint64_t&);
template bool DataManager::FetchValue<double>(const std::string&, double&);
template bool DataManager::FetchValue<bool>(const std::string&, bool&);
template bool DataManager::FetchValue<std::string>(const std::string&, std::string&);

} // namespace diagnostic_manager
