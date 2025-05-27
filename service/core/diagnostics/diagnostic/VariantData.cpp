#include "VariantData.hpp"

namespace diagnostic_manager {

void VariantData::SetName(const std::string& name) {
    name_ = name;
}

std::string VariantData::GetName() {
    return name_;
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
