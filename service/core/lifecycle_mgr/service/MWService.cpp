#include "MWService.hpp"

namespace lifecycle_manager {

MWService::MWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args) :
    code_(code),
    active_(active),
    pid_(-1),
    mw_service_name_(name),
    mw_service_path_(exepath),
    mw_run_args_(args)
{

}


bool MWService::IsActive(){
    return active_;
}

std::string MWService::GetName(){
    return mw_service_name_;
}

std::string MWService::GetPath(){
    return mw_service_path_;
}

std::vector<std::string> MWService::GetArgs(){
    
    std::vector<std::string> args;

    for (auto& arg : mw_run_args_){
        args.push_back(arg.GetArgument());
    }
    return args;
}


void MWService::SetPID(pid_t pid){
    pid_ = pid;
}

pid_t MWService::GetPID(){
    return pid_;
} 


} // namespace lifecycle_manager
