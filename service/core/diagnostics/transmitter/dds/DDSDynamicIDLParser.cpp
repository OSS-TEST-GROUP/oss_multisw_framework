#include "DDSDynamicIDLParser.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>

#include <filesystem>

namespace diagnostic_manager {

namespace fs = std::filesystem;

bool DDSDynamicIDLParser::Initialize(const std::string& idl_directory)
{
    if (!fs::exists(idl_directory) || !fs::is_directory(idl_directory))
    {
        std::cerr << "IDL directory not found or is not a directory: " << idl_directory << std::endl;
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

    for (const auto& entry : fs::directory_iterator(idl_directory_))
    {
        if (entry.path().extension() == ".idl")
        {
            auto type = Parse(entry.path().filename().string());
            if (type)
            {
                types.push_back(type);
            }
        }
    }

    return types;
}

std::shared_ptr<DynamicIDLTypeSupport> DDSDynamicIDLParser::Parse(const std::string& idl_name)
{
    std::string idl_path = (fs::path(idl_directory_) / idl_name).string();

    // For simplicity, we assume the IDL contains only one top-level type
    // and its name matches the file name (without extension)
    std::string type_name = idl_name.substr(0, idl_name.find_last_of('.'));

    auto builder = DynamicTypeBuilderFactory::get_instance()->create_type_w_uri(idl_path, type_name, {idl_directory_});
    if (!builder)
    {
        std::cerr << "Failed to parse IDL: " << idl_path << std::endl;
        return nullptr;
    }

    auto dynamic_idl_type = std::make_shared<DynamicIDLTypeSupport>();
    dynamic_idl_type->dyn_type = builder->build();
    dynamic_idl_type->type_support = std::make_shared<TypeSupport>(new DynamicPubSubType(dynamic_idl_type->dyn_type));
    return dynamic_idl_type;
}

} // namespace diagnostic_manager