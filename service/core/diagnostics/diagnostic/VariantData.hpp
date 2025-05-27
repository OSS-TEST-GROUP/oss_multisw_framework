#ifndef DIAGNOSTICS_DIAGNOSTIC_VARIANT_DATA_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_VARIANT_DATA_HPP_

#include <iostream>
#include <vector>
#include <typeindex>
#include <variant>

namespace diagnostic_manager {

class VariantData;

using VariantType = std::variant<
    char,
    wchar_t,
    int8_t,
    int16_t,
    int32_t,
    int64_t,
    uint8_t,
    uint16_t,
    uint32_t,
    uint64_t,
    float,
    double,
    bool,
    std::string,
    std::wstring,
    VariantData,
    std::vector<char>,
    std::vector<wchar_t>,
    std::vector<int8_t>,
    std::vector<int16_t>,
    std::vector<int32_t>,
    std::vector<int64_t>,
    std::vector<uint8_t>,
    std::vector<uint16_t>,
    std::vector<uint32_t>,
    std::vector<uint64_t>,
    std::vector<float>,
    std::vector<double>,
    std::vector<bool>,
    std::vector<std::string>,
    std::vector<std::wstring>,
    std::vector<VariantData>
>;

class VariantData {
  public:
    void SetName(const std::string& name);
    std::string GetName();

    template<typename T>
    bool SetValue(std::string key, T value) {
        members_[key] = value;
        return true;
    }

    template<typename T>
    T GetValue(const std::string& name) {
        return std::get<T>(members_[name]);
    }

    std::type_index GetValueType(const std::string& name);

    bool HasKey(const std::string& name);
    std::vector<std::string> GetKeyList();

    uint64_t GetValueNum();

  private:
    std::string name_;
    std::map<std::string, VariantType> members_;
};

} // namespace diagnostic_manager

#endif
