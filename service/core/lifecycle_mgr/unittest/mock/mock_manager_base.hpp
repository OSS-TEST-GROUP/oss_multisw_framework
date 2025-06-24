#ifndef MOCK_LIFECYCLE_MANAGER_BASE_HPP_
#define MOCK_LIFECYCLE_MANAGER_BASE_HPP_

#include <gmock/gmock.h>
#include "manager/ManagerBase.hpp"

namespace lifecycle_manager {

class MockManagerBase : public ManagerBase {
public:
    MockManagerBase(LifecycleManager& lifecycle) : ManagerBase(lifecycle) {}    
    virtual ~MockManagerBase() {}

    // Mock methods for ManagerBase
    MOCK_METHOD(std::shared_ptr<DDSTransmitter>, GetTransmitter, (), ());
    MOCK_METHOD(std::shared_ptr<Service>, GetService, (std::string name), ());
    MOCK_METHOD(std::shared_ptr<ServiceRegistry>, GetRegistry, (), ());
    MOCK_METHOD(std::shared_ptr<PolicyManager>, GetPolicy, (), ());
    MOCK_METHOD(const char*, GetTopic, (MessageType mtype), ());

    // Add more mocks as needed

};

MockManagerBase* Mock_ManagerBase;  

ManagerBase::ManagerBase(LifecycleManager& lifecycle) : lifecycle_(lifecycle) {}
ManagerBase::~ManagerBase() {}
std::shared_ptr<DDSTransmitter> ManagerBase::GetTransmitter() {
    return Mock_ManagerBase->GetTransmitter();
}
std::shared_ptr<Service> ManagerBase::GetService(std::string name) {
    return Mock_ManagerBase->GetService(name);
}
std::shared_ptr<ServiceRegistry> ManagerBase::GetRegistry() {
    return Mock_ManagerBase->GetRegistry();
}
std::shared_ptr<PolicyManager> ManagerBase::GetPolicy() {
    return Mock_ManagerBase->GetPolicy();
}
const char* ManagerBase::GetTopic(MessageType mtype) {
    return Mock_ManagerBase->GetTopic(mtype);
}


} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_MANAGER_BASE_HPP_