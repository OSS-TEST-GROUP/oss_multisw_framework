#ifndef DIAGNOSTICS_TRASMITTER_DDS_DYNAMIC_IDL_PARSER_HPP_
#define DIAGNOSTICS_TRASMITTER_DDS_DYNAMIC_IDL_PARSER_HPP_

#include <iostream>
#include <unordered_map>
#include <vector>

#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicType.hpp"

using namespace eprosima::fastdds::dds;

namespace diagnostic_manager {

class DynamicIDLTypeSupport {
    DynamicType::_ref_type dyn_type;
    std::shared_ptr<TypeSupport> type_support;
};

class DDSDynamicIDLParser {
  public:
    bool Initialize(const std::string& idl_directory);

    std::vector<std::shared_ptr<DynamicIDLTypeSupport>> ParseAll();
    std::shared_ptr<DynamicIDLTypeSupport> Parse(const std::string& idl_name);

  private:
    std::string idl_directory_;
};

} // namespace diagnostic_manager

#endif
