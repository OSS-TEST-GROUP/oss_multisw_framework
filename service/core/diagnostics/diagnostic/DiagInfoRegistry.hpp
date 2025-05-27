#ifndef DIAGNOSTICS_DIAGNOSTIC_DIAG_INFO_REGISTRY_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_DIAG_INFO_REGISTRY_HPP_

#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

#include "DiagInfo.hpp"

namespace diagnostic_manager {

class DiagInfoRegistry {
  public:
    DiagInfoRegistry();
    virtual ~DiagInfoRegistry();

    void UpdateDiagInfo(std::shared_ptr<DiagInfo> diag_info);
    void RemoveDiagInfo(int32_t code);

    void ClearDiagInfoList();

    std::shared_ptr<DiagInfo> GetDiagInfo(int32_t code);
    std::shared_ptr<DiagInfo> GetDiagInfoByName(std::string name);
    std::vector<std::shared_ptr<DiagInfo>> GetAllDiagInfo();

    std::vector<int32_t> GetDiagInfoCodeList();
    std::vector<std::string> GetDiagInfoNameList();

  private:
    std::unordered_map<int32_t, std::shared_ptr<DiagInfo>> diag_info_list_;
    std::mutex diag_info_mutex_;
};

} // namespace diagnostic_manager

#endif
