#ifndef MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_
#define MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_

#include <gmock/gmock.h>
#include "service/MWServiceManager.hpp"
#include "service/MWService.hpp"
#include "service/ArgumentConfig.hpp"
#include <sys/types.h>

namespace lifecycle_manager {
class MockMWServiceManager : public MWServiceManager {
public:
    // Only mock virtual methods
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(bool, Initialize, (), ());
    MOCK_METHOD(bool, LoadMWConfig, (std::string path), ());
    MOCK_METHOD(void, StartMWService, (std::shared_ptr<MWService> mw_service), ());
    MOCK_METHOD(void, StopMWService, (std::shared_ptr<MWService> mw_service), ());
    MOCK_METHOD(void, RunMWServiceManagement, (), ());
    MOCK_METHOD(pid_t, run_process, (const std::string exename, const std::string exepath, const std::vector<std::string>& args), ());
    MOCK_METHOD(bool, kill_process, (pid_t pid), ());
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


void MWServiceManager::StartMWService(std::shared_ptr<MWService> mw_service) {
    Mock_MWServiceManager->StartMWService(mw_service);
}

void MWServiceManager::StopMWService(std::shared_ptr<MWService> mw_service) {
    Mock_MWServiceManager->StopMWService(mw_service);
}

void MWServiceManager::RunMWServiceManagement() {
    return Mock_MWServiceManager->RunMWServiceManagement();
}

pid_t MWServiceManager::run_process(const std::string exename, const std::string exepath, const std::vector<std::string>& args) {
    // Forward to mock if available; otherwise return error
    if (Mock_MWServiceManager) {
        return Mock_MWServiceManager->run_process(exename, exepath, args);
    }
    return -1;
}

bool MWServiceManager::kill_process(pid_t pid) {
    if (Mock_MWServiceManager) {
        return Mock_MWServiceManager->kill_process(pid);
    }
    return false;
}

bool MWServiceManager::IsK3sMode() const {
    if (Mock_MWServiceManager) {
        return Mock_MWServiceManager->IsK3sMode();
    }
    return false;
}
} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MW_SERVICE_MANAGER_HPP_