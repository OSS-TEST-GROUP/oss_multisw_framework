#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DATA_MANAGER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DATA_MANAGER_HPP_

#include <map>
#include <set>
#include <memory>
#include <string>
#include <mutex>
#include "diagnostic/VariantData.hpp"

namespace diagnostic_manager {

class DataManager {
public:
    bool UpdateData(std::shared_ptr<VariantData> variant_data);

    std::shared_ptr<VariantData> GetData(const std::string& name);
    std::set<std::shared_ptr<VariantData>> GetAllUpdatedData();

    template<typename T>
    bool FetchValue(const std::string& path, T& out);
    bool FetchValueAsVariant(const std::string& path, VariantValue& value);

private:
    std::vector<std::string> GetPathTokens(const std::string& path);
    bool GetIndiciesInfo(const std::string& token, std::string& key, std::vector<size_t>& indices);

    VariantData* FindChildByExploring(std::shared_ptr<VariantData> root, std::vector<std::string> tokens);

    std::map<std::string, std::shared_ptr<VariantData>> data_list_;
    std::set<std::shared_ptr<VariantData>> updated_data_list_;
    std::mutex data_handling_lock_;
};

}  // namespace diagnostic_manager

#endif // DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DATA_MANAGER_HPP_
