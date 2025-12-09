#ifndef MOCK_LIFECYCLE_MW_SERVICE_HPP_
#define MOCK_LIFECYCLE_MW_SERVICE_HPP_

#include <gmock/gmock.h>
#include "service/MWService.hpp"

namespace lifecycle_manager {

class MockMWService : public MWService {
public:
    MockMWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args)
        : MWService(code, name, exepath, active, args) { lifecycle_manager::gMockMWService = this; }
    virtual ~MockMWService() { lifecycle_manager::gMockMWService = nullptr; }

    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(std::string, GetPath, (), ());
    MOCK_METHOD(std::vector<std::string>, GetArgs, (), ());
    MOCK_METHOD(void, SetPID, (pid_t), ());
    MOCK_METHOD(pid_t, GetPID, (), ());
    MOCK_METHOD(bool, IsActive, (), ());
};

inline MockMWService* gMockMWService = nullptr;


MWService::MWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args){}

MWService::~MWService() {}

bool MWService::IsActive()  {
    return Mock_MWService->IsActive();
}
std::string MWService::GetName()  {
    return Mock_MWService->GetName();

}
std::string MWService::GetPath()  {
    return Mock_MWService->GetPath();
}
std::vector<std::string> MWService::GetArgs()  {
    return Mock_MWService->GetArgs();   
}

void MWService::SetPID(pid_t pid) {
    Mock_MWService->SetPID(pid);        
}
pid_t MWService::GetPID()  {
    return Mock_MWService->GetPID();
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MW_SERVICE_HPP_