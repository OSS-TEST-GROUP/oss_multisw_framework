#pragma once
#include <gmock/gmock.h>

#include "service/MWService.hpp"

namespace lifecycle_manager {

class MockMWService : public MWService {
public:
    MOCK_METHOD(std::string, GetName, (), (const, ));
    // Add more mocked methods as needed based on MWService interface
    MOCK_METHOD(std::string, GetPath, (), (const, ));
    MOCK_METHOD(std::vector<std::string>, GetArgs, (), (const, ));
    MOCK_METHOD(void, SetPID, (pid_t), ());
    MOCK_METHOD(pid_t, GetPID, (), (const));
    MOCK_METHOD(bool, IsActive, (), (const));

};


MockMWService* Mock_MWService;

MWService::MWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args){}

MWService::~MWService() {}

bool MWService::IsActive() const {
    return Mock_MWService->IsActive();
}
std::string MWService::GetName() const {
    return Mock_MWService->GetName();

}
std::string MWService::GetPath() const {
    return Mock_MWService->GetPath();
}
std::vector<std::string> MWService::GetArgs() const {
    return Mock_MWService->GetArgs();   
}

void MWService::SetPID(pid_t pid) {
    Mock_MWService->SetPID(pid);        
}
pid_t MWService::GetPID() const {
    return Mock_MWService->GetPID();    
}

} // namespace lifecycle_manager