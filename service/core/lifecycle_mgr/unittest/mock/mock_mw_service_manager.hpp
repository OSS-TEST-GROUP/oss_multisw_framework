#ifndef MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_
#define MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_

#include <gmock/gmock.h>

#include <sys/types.h>

#include "service/MWServiceManager.hpp"
#include "service/MWService.hpp"
#include "service/ArgumentConfig.hpp"

namespace lifecycle_manager {

// Mock for the post-F011 MWServiceManager surface (no fork/exec helpers).
class MockMWServiceManager : public MWServiceManager {
public:
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(bool, LoadMWConfig, (std::string path), ());
    MOCK_METHOD(bool, IsK3sMode, (), (const));
};

inline MockMWServiceManager* Mock_MWServiceManager = nullptr;

MWServiceManager::MWServiceManager() {}
// MWServiceManager::~MWServiceManager() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

bool MWServiceManager::Initialize() {
    return Mock_MWServiceManager->Initialize();
}

bool MWServiceManager::Start() {
    return Mock_MWServiceManager->Start();
}

void MWServiceManager::Stop() {
    Mock_MWServiceManager->Stop();
}

bool MWServiceManager::LoadMWConfig(const std::string& path) {
    return Mock_MWServiceManager->LoadMWConfig(path);
}

bool MWServiceManager::IsK3sMode() const {
    if (Mock_MWServiceManager) {
        return Mock_MWServiceManager->IsK3sMode();
    }
    return false;
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_
