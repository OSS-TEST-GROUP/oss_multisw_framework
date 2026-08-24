#ifndef MOCK_LIFECYCLE_SERVICE_REGISTRY_HPP_
#define MOCK_LIFECYCLE_SERVICE_REGISTRY_HPP_

#include <gmock/gmock.h>
#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/Container.hpp"
#include "include/ContainerState.hpp"

namespace lifecycle_manager {

class MockServiceRegistry : public ServiceRegistry {
public:
    // Only mock virtual methods
    MOCK_METHOD(std::shared_ptr<lifecycle_manager::Service>, GetService, (std::string), ());
    MOCK_METHOD(bool, AddService, (std::shared_ptr<lifecycle_manager::Service>, std::string), ());
    MOCK_METHOD(void, RemoveService, (std::string), ());
    MOCK_METHOD(std::vector<std::shared_ptr<lifecycle_manager::Service>>, GetServicesByGroup, (std::string), ());
    MOCK_METHOD(std::vector<std::string>, GetServicesNameByGroup, (std::string), ());
    MOCK_METHOD(std::string, GetGroupNameByService, (std::shared_ptr<lifecycle_manager::Service>), ());
    MOCK_METHOD(uint64_t, GetServiceNum, (), ());
    MOCK_METHOD(uint64_t, GetServiceNumInGroup, (std::string), ());
    MOCK_METHOD(uint64_t, GetGroupNum, (), ());
    MOCK_METHOD(bool, UpdateServiceList, (UpdateServiceListRequest*), ());
    MOCK_METHOD(void, AddGroup, (std::string), ());
    MOCK_METHOD(void, RemoveGroup, (std::string), ());
    MOCK_METHOD(bool, IsGroup, (std::string), ());

    MOCK_METHOD(std::shared_ptr<std::vector<std::string>>, getGroup, (std::string), ());
    MOCK_METHOD(bool, hasGroup, (std::string), ());
    MOCK_METHOD(bool, vectorAdd, (std::vector<std::string>&, std::string), ());
    MOCK_METHOD(bool, vectorRemove, (std::vector<std::string>&, std::string), ());
    MOCK_METHOD(bool, vectorHas, (std::vector<std::string>, std::string), ());
    MOCK_METHOD(bool, hasService, (std::string), ());
    MOCK_METHOD(std::shared_ptr<lifecycle_manager::Service>, getService, (std::string), ());
    MOCK_METHOD(std::vector<std::string>, GetGroupList, (), ());

    // F013: container surface
    MOCK_METHOD(std::shared_ptr<lifecycle_manager::Container>, GetContainer, (const std::string&), ());
    MOCK_METHOD(std::vector<std::shared_ptr<lifecycle_manager::Container>>, GetAllContainers, (), ());
    MOCK_METHOD(void, UpsertContainer, (const std::shared_ptr<lifecycle_manager::Container>&), ());
    MOCK_METHOD(bool, UpdateContainerState, (const std::string&, lifecycle_manager::ContainerState, int32_t), ());
    MOCK_METHOD(std::vector<std::string>, GetServicesInContainer, (const std::string&), ());
};

inline MockServiceRegistry* Mock_ServiceRegistry = nullptr;


ServiceRegistry::ServiceRegistry() {}
// ServiceRegistry::~ServiceRegistry() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
// 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
// defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.

bool ServiceRegistry::AddService(const std::shared_ptr<Service>& service, std::string group) {
    return Mock_ServiceRegistry->AddService(service, group);
}
void ServiceRegistry::RemoveService(const std::string& name) {
    Mock_ServiceRegistry->RemoveService(name);
}
std::shared_ptr<Service> ServiceRegistry::GetService(const std::string& name) {
    return Mock_ServiceRegistry->GetService(name);  
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::GetServicesByGroup(const std::string& group) {
    return Mock_ServiceRegistry->GetServicesByGroup(group);
}
std::vector<std::string> ServiceRegistry::GetServicesNameByGroup(const std::string& group) {
    return Mock_ServiceRegistry->GetServicesNameByGroup(group);
}

std::string ServiceRegistry::GetGroupNameByService(const std::shared_ptr<Service>& service) {
    return Mock_ServiceRegistry->GetGroupNameByService(service);
}
uint64_t ServiceRegistry::GetServiceNum() {
    return Mock_ServiceRegistry->GetServiceNum();
}
uint64_t ServiceRegistry::GetServiceNumInGroup(const std::string& group) {
    return Mock_ServiceRegistry->GetServiceNumInGroup(group);
}
uint64_t ServiceRegistry::GetGroupNum() {
    return Mock_ServiceRegistry->GetGroupNum();
}
bool ServiceRegistry::UpdateServiceList(UpdateServiceListRequest* update) {
    return Mock_ServiceRegistry->UpdateServiceList(update);
}
void ServiceRegistry::AddGroup(const std::string& name) {
    Mock_ServiceRegistry->AddGroup(name);
}
void ServiceRegistry::RemoveGroup(const std::string& name) {
    Mock_ServiceRegistry->RemoveGroup(name);
}
bool ServiceRegistry::IsGroup(std::string name) {
    return Mock_ServiceRegistry->IsGroup(name);
}
std::shared_ptr<std::vector<std::string>> ServiceRegistry::getGroup(const std::string& name) {
    return Mock_ServiceRegistry->getGroup(name);
}
bool ServiceRegistry::hasGroup(const std::string& name) {
    return Mock_ServiceRegistry->hasGroup(name);
}

bool ServiceRegistry::vectorAdd(std::vector<std::string>& vec, const std::string& item) {
    return Mock_ServiceRegistry->vectorAdd(vec, item);
}
bool ServiceRegistry::vectorRemove(std::vector<std::string>& vec, const std::string& item) {
    return Mock_ServiceRegistry->vectorRemove(vec, item);   
}   

bool ServiceRegistry::vectorHas(std::vector<std::string> vec, const std::string& item) {
    return Mock_ServiceRegistry->vectorHas(vec, item);
}
bool ServiceRegistry::hasService(const std::string& name) {
    return Mock_ServiceRegistry->hasService(name);
}
std::shared_ptr<Service> ServiceRegistry::getService(const std::string& name) {
    return Mock_ServiceRegistry->getService(name);
}      

std::vector<std::string> ServiceRegistry::GetGroupList(){
    return Mock_ServiceRegistry->GetGroupList();
}

// F013: container surface — mock-forwarded so LifecycleManagerTest links
// without dragging in the real ServiceRegistry.cpp.
std::shared_ptr<Container> ServiceRegistry::GetContainer(const std::string& name){
    return Mock_ServiceRegistry ? Mock_ServiceRegistry->GetContainer(name) : nullptr;
}
std::vector<std::shared_ptr<Container>> ServiceRegistry::GetAllContainers(){
    return Mock_ServiceRegistry ? Mock_ServiceRegistry->GetAllContainers()
                                : std::vector<std::shared_ptr<Container>>{};
}
void ServiceRegistry::UpsertContainer(const std::shared_ptr<Container>& c){
    if (Mock_ServiceRegistry) Mock_ServiceRegistry->UpsertContainer(c);
}
bool ServiceRegistry::UpdateContainerState(const std::string& name,
                                            ContainerState state,
                                            int32_t exit_code){
    return Mock_ServiceRegistry
        ? Mock_ServiceRegistry->UpdateContainerState(name, state, exit_code)
        : false;
}
std::vector<std::string> ServiceRegistry::GetServicesInContainer(const std::string& name){
    return Mock_ServiceRegistry ? Mock_ServiceRegistry->GetServicesInContainer(name)
                                : std::vector<std::string>{};
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_SERVICE_REGISTRY_HPP_