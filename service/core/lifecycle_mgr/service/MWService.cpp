#include "MWService.hpp"

namespace lifecycle_manager {

MWService::MWService() {}
MWService::~MWService() {}


std::string MWService::GetName(){
    return mw_service_name_;
}

std::string MWService::GetPath(){
    return mw_service_path_;
}

std::vector<std::string> MWService::GetArgs(){
    return mw_run_args_;
}

} // namespace lifecycle_manager
