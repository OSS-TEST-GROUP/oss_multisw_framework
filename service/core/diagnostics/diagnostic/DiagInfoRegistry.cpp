#include "DiagInfoRegistry.hpp"

namespace diagnostic_manager {

DiagInfoRegistry::DiagInfoRegistry() {}
DiagInfoRegistry::~DiagInfoRegistry() {}

void DiagInfoRegistry::UpdateDiagInfo(std::shared_ptr<DiagInfo> diag_info) {
    std::lock_guard<std::mutex> lock_guard(diag_info_mutex_);

    if (diag_info == nullptr) {
        return;
    }
    
    int32_t diag_code = diag_info->GetDiagCode();
    auto iter = diag_info_list_.find(diag_code);
    if (iter == diag_info_list_.end()) {
        diag_info_list_[diag_code] = diag_info;
    } else {
        if (*(iter->second) != *diag_info) {
            diag_info_list_[diag_code] = diag_info;
        }
    }
}

void DiagInfoRegistry::RemoveDiagInfo(int32_t code) {
    std::lock_guard<std::mutex> lock_guard(diag_info_mutex_);
    
    if (diag_info_list_.count(code) == 0) {
        return;
    }
    diag_info_list_.erase(code);
}

void DiagInfoRegistry::ClearDiagInfoList() {
    std::lock_guard<std::mutex> lock_guard(diag_info_mutex_);

    diag_info_list_.clear();
}

std::shared_ptr<DiagInfo> DiagInfoRegistry::GetDiagInfo(int32_t code) {
    std::lock_guard<std::mutex> lock_guard(diag_info_mutex_);

    auto iter = diag_info_list_.find(code);
    if (iter == diag_info_list_.end()) {
        return nullptr;
    }
    return iter->second;
}
std::shared_ptr<DiagInfo> DiagInfoRegistry::GetDiagInfoByName(const std::string& name) {
    std::lock_guard<std::mutex> lock_guard(diag_info_mutex_);

    for (auto& iter : diag_info_list_) {
        if (iter.second->GetDiagName() == name) {
            return iter.second;
        }
    }
    return nullptr;
}
std::vector<std::shared_ptr<DiagInfo>> DiagInfoRegistry::GetAllDiagInfo() {
    std::vector<std::shared_ptr<DiagInfo>> diag_list;
    diag_list.reserve(diag_info_list_.size());
    std::transform(diag_info_list_.begin(), diag_info_list_.end(), std::back_inserter(diag_list),
                   [](const auto& iter) { return iter.second; });
    return diag_list;
}

std::vector<int32_t> DiagInfoRegistry::GetDiagInfoCodeList() {
    std::vector<int32_t> code_list;
    code_list.reserve(diag_info_list_.size());
    std::transform(diag_info_list_.begin(), diag_info_list_.end(), std::back_inserter(code_list),
                   [](const auto& iter) { return iter.first; });
    return code_list;
}
std::vector<std::string> DiagInfoRegistry::GetDiagInfoNameList() {
    std::vector<std::string> name_list;
    name_list.reserve(diag_info_list_.size());
    std::transform(diag_info_list_.begin(), diag_info_list_.end(), std::back_inserter(name_list),
                   [](const auto& iter) { return iter.second->GetDiagName(); });

    return name_list;
}

} // namespace diagnostic_manager
