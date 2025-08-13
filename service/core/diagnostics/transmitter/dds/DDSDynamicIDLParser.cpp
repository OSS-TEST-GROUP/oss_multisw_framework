#include "DDSDynamicIDLParser.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

#include "logger/Log.hpp"

namespace diagnostic_manager {

namespace fs = std::filesystem;

bool DDSDynamicIDLParser::Initialize(const std::string& idl_directory)
{
    if (!fs::exists(idl_directory) || !fs::is_directory(idl_directory))
    {
        LOG_ERROR("IDL directory not found or is not a directory: " << idl_directory);
        return false;
    }

    idl_directory_ = idl_directory;
    return true;
}

std::vector<std::shared_ptr<DynamicIDLTypeSupport>> DDSDynamicIDLParser::ParseAll()
{
    std::vector<std::shared_ptr<DynamicIDLTypeSupport>> types;
    if (idl_directory_ == "") {
        return types;
    }

    for (const auto& entry : fs::recursive_directory_iterator(idl_directory_))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".idl")
        {
            // auto ret = Parse(entry.path().filename().string());
            // types.insert(types.end(), ret.begin(), ret.end());
            std::filesystem::path relative_path = std::filesystem::relative(entry.path(), idl_directory_);
            auto ret = Parse(relative_path.string());
            types.insert(types.end(), ret.begin(), ret.end());
        }
    }

    return types;
}

std::vector<std::shared_ptr<DynamicIDLTypeSupport>> DDSDynamicIDLParser::Parse(const std::string& idl_name)
{
    std::vector<std::shared_ptr<DynamicIDLTypeSupport>> types;
    std::string idl_path = (fs::path(idl_directory_) / idl_name).string();

    auto type_list = ExtractStructNamesFromIDL(idl_name);

    for (auto& iter : type_list) {
        auto builder = DynamicTypeBuilderFactory::get_instance()->create_type_w_uri(idl_path, iter, {idl_directory_});
        if (!builder)
        {
            LOG_ERROR("Failed to parse IDL: " << idl_path);
            continue;
        }

        auto dynamic_idl_type = std::make_shared<DynamicIDLTypeSupport>();
        dynamic_idl_type->dyn_type = builder->build();
        dynamic_idl_type->type_support = std::make_shared<TypeSupport>(new DynamicPubSubType(dynamic_idl_type->dyn_type));
        types.push_back(dynamic_idl_type);
    }

    return types;
}

std::vector<std::string> DDSDynamicIDLParser::ExtractStructNamesFromIDL(const std::string& idl_name) {
    std::string idl_path = (fs::path(idl_directory_) / idl_name).string();
    std::ifstream file(idl_path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open IDL file: " << idl_path);
        return {};
    }

    std::vector<std::string> results;
    std::regex struct_decl(R"(\s*struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(\{)?)");

    std::string line;
    while (std::getline(file, line)) {
        std::smatch match;
    
        // Struct declaration
        if (std::regex_search(line, match, struct_decl)) {
            results.push_back(match[1]);
            continue;
        }
    }

    return results;
}

} // namespace diagnostic_manager