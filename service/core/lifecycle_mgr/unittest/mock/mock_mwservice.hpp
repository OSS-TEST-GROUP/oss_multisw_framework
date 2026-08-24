#ifndef MOCK_LIFECYCLE_MW_SERVICE_HPP_
#define MOCK_LIFECYCLE_MW_SERVICE_HPP_

#include <gmock/gmock.h>
#include "service/MWService.hpp"

namespace lifecycle_manager {

class MockMWService : public MWService {
public:
    MockMWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args)
        : MWService(code, name, exepath, active, args) {  }
    virtual ~MockMWService() { }

    MOCK_METHOD(std::string, GetName, (), ());
    MOCK_METHOD(std::string, GetPath, (), ());
    MOCK_METHOD(std::vector<std::string>, GetArgs, (), ());
    MOCK_METHOD(void, SetPID, (pid_t), ());
    MOCK_METHOD(pid_t, GetPID, (), ());
    MOCK_METHOD(bool, IsActive, (), ());
};

inline MockMWService* Mock_MWService = nullptr;


MWService::MWService(int32_t code, const std::string& name, const std::string& exepath, bool active, const std::vector<ArgumentConfig>& args){}

// MWService::~MWService() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

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