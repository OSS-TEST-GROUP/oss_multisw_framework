#include "ArgumentConfig.hpp"

namespace lifecycle_manager {

ArgumentConfig::ArgumentConfig(const std::string& name, const std::string& description, const std::string& type, const std::string& default_value) :
    arg_name_(name), arg_description_(description), arg_type_(type), arg_default_(default_value){
    
}

ArgumentConfig::~ArgumentConfig() {}

std::string ArgumentConfig::GetName(){
    return arg_name_;
}


std::string ArgumentConfig::GetDescription(){
    return arg_description_;
}

std::string ArgumentConfig::GetType(){
    return arg_type_;
}


std::string ArgumentConfig::GetArgument(){
    return arg_default_;
}


} // namespace lifecycle_manager
