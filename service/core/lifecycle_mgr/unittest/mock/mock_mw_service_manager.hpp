// Google Mock for MWServiceManager
#pragma once
#include <gmock/gmock.h>
#include "service/MWServiceManager.hpp"
#include "service/MWService.hpp"
#include "service/ArgumentConfig.hpp"

namespace lifecycle_manager {
class MockMWServiceManager : public MWServiceManager {
public:
    MOCK_METHOD(bool, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(bool, Initialize, (), ());
    // Add more mocks as needed

    MOCK_METHOD(bool, LoadMWConfig, (std::string path), ());
    MOCK_METHOD(void, StartMWService, (std::shared_ptr<MWService> mw_service), ());
    MOCK_METHOD(void, StopMWService, (std::shared_ptr<MWService> mw_service), ());
    MOCK_METHOD(void, RunMWServiceManagement, (), ());

};


MockMWServiceManager* Mock_MWServiceManager;

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


bool MWServiceManager::StartMWService(std::shared_ptr<MWService> mw_service) {
    return Mock_MWServiceManager->StartMWService(mw_service);
}

void MWServiceManager::StopMWService(std::shared_ptr<MWService> mw_service) {
    Mock_MWServiceManager->StopMWService(mw_service);
}

void MWServiceManager::RunMWServiceManagement() {
    return Mock_MWServiceManager->RunMWServiceManagement();
}

} // namespace lifecycle_manager
