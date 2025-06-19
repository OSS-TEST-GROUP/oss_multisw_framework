#ifndef LIFECYCLE_MW_SERVICE_ARGUMENT_HPP_
#define LIFECYCLE_MW_SERVICE_ARGUMENT_HPP_

#include <string>
#include <vector>

namespace lifecycle_manager {

class ArgumentConfig {
  public:
    ArgumentConfig(const std::string& name, const std::string& description, const std::string& type, const std::string& default_value);
    virtual ~ArgumentConfig();

    std::string GetName();
    std::string GetDescription();
    std::string GetType();
    std::string GetArgument();
 
  private:
    std::string arg_name_;
    std::string arg_description_;
    std::string arg_type_;
    std::string arg_default_;
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_MW_SERVICE_ARGUMENT_HPP_
