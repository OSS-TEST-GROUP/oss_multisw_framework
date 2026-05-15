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
MWServiceManager::~MWServiceManager() {}

bool MWServiceManager::Initialize() {
    return Mock_MWServiceManager->Initialize();
}

bool MWServiceManager::Start() {
    return Mock_MWServiceManager->Start();
}

void MWServiceManager::Stop() {
    Mock_MWServiceManager->Stop();
}

bool MWServiceManager::LoadMWConfig(std::string path) {
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
