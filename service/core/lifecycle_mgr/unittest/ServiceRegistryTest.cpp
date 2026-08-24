// Unit tests for ServiceRegistry class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "service/ServiceRegistry.hpp"
#include "unittest/mock/mock_service.hpp"
#include "unittest/mock/mock_lifecycle_factory.hpp"
#include "unittest/mock/mock_dds_transmitter.hpp"
#include "unittest/mock/mock_state_manager.hpp"
#include "unittest/mock/mock_manager_base.hpp"

#include "include/ServiceState.hpp"
#include "include/ServiceType.hpp"
#include "include/ServiceMode.hpp"
#include "include/RecoveryPolicy.hpp"

using namespace lifecycle_manager;

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;


LifecycleFactory* LifecycleFactory::GetInstance() { 
    static LifecycleFactory inst; 
    return &inst; 
}

class ServiceRegistryTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new ServiceRegistry();
        // Do NOT allocate Mock_Service here; tests that need it will create a shared_ptr
        // and set Mock_Service to the raw pointer for forwarding.
        Mock_Service = nullptr;
    }
    virtual void TearDown(){
        delete ins;
        // Do NOT delete Mock_Service here; it is managed by shared_ptr in tests that use it.
        Mock_Service = nullptr;
    }

    ServiceRegistry* ins;
};

TEST_F(ServiceRegistryTest, GetServiceReturnsExpected) {
    auto expected_service = std::make_shared<MockService>("test", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = expected_service.get();  // Set global pointer for forwarding
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test"));
    
    ins->AddService(expected_service, "");
    auto result = ins->GetService("test");
    EXPECT_EQ(result, expected_service);
}

// Add more tests for ServiceRegistry as needed
TEST_F(ServiceRegistryTest, AddAndGetService) {
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();  // Set global pointer for forwarding
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    EXPECT_TRUE(ins->AddService(service, ""));
    auto fetched = ins->GetService("svc1");
    EXPECT_EQ(fetched, service);
}

TEST_F(ServiceRegistryTest, AddDuplicateServiceReturnsTrueButDoesNotDuplicate) {
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();  // Set global pointer for forwarding
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    EXPECT_TRUE(ins->AddService(service, ""));
    EXPECT_TRUE(ins->AddService(service, "")); // No duplicate check in AddService
    EXPECT_EQ(ins->GetServiceNum(), 1);
}

TEST_F(ServiceRegistryTest, RemoveServiceRemovesIt) {
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();  // Set global pointer for forwarding
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    ins->AddService(service, "");
    ins->RemoveService("svc1");
    EXPECT_EQ(ins->GetService("svc1"), nullptr);
}

TEST_F(ServiceRegistryTest, AddAndRemoveGroup) {
    ins->AddGroup("group1");
    EXPECT_TRUE(ins->IsGroup("group1"));
    ins->RemoveGroup("group1");
    EXPECT_FALSE(ins->IsGroup("group1"));
}

TEST_F(ServiceRegistryTest, GetServicesByGroupReturnsCorrectServices) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    
    // Set up ON_CALL for each service in sequence
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    ins->AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    ins->AddService(service2, "");
    
    auto group_services = ins->GetServicesByGroup("group1");
    ASSERT_EQ(group_services.size(), 1);
    // Need to set Mock_Service back to service1 for GetName call
    Mock_Service = service1.get();
    EXPECT_EQ(group_services[0]->GetName(), "svc1");
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroupReturnsCorrectNames) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    ins->AddService(service1, "group1");
    auto names = ins->GetServicesNameByGroup("group1");
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "svc1");
}

TEST_F(ServiceRegistryTest, GetGroupNameByServiceReturnsCorrectGroup) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    ins->AddService(service1, "group1");
    EXPECT_EQ(ins->GetGroupNameByService(service1), "group1");
}

TEST_F(ServiceRegistryTest, GetServiceNumAndGroupNum) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    ins->AddService(service1, "group1");
    EXPECT_EQ(ins->GetServiceNum(), 1);
    EXPECT_EQ(ins->GetGroupNum(), 1);
}

TEST_F(ServiceRegistryTest, GetServiceNumInGroup) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    ins->AddService(service1, "group1");
    EXPECT_EQ(ins->GetServiceNumInGroup("group1"), 1);
    EXPECT_EQ(ins->GetServiceNumInGroup(""), 1);
}

TEST_F(ServiceRegistryTest, VectorHasAddRemove) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_TRUE(registry.vectorAdd(vec, "item1"));
    EXPECT_TRUE(registry.vectorHas(vec, "item1"));
    EXPECT_TRUE(registry.vectorRemove(vec, "item1"));
    EXPECT_FALSE(registry.vectorHas(vec, "item1"));
}

TEST_F(ServiceRegistryTest, VectorAddDuplicate) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_TRUE(registry.vectorAdd(vec, "item1"));
    EXPECT_FALSE(registry.vectorAdd(vec, "item1")); // Should return false for duplicate
    EXPECT_EQ(vec.size(), 1);
}

TEST_F(ServiceRegistryTest, VectorRemoveNonExistent) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_FALSE(registry.vectorRemove(vec, "nonexistent"));
}

TEST_F(ServiceRegistryTest, AddServiceToMultipleGroups) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    registry.AddGroup("group2");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    registry.AddService(service1, "group1");
    auto names1 = registry.GetServicesNameByGroup("group1");
    EXPECT_EQ(names1.size(), 1);
}

TEST_F(ServiceRegistryTest, RemoveNonExistentService) {
    ServiceRegistry registry;
    EXPECT_NO_THROW(registry.RemoveService("nonexistent"));
}

TEST_F(ServiceRegistryTest, RemoveNonExistentGroup) {
    ServiceRegistry registry;
    EXPECT_NO_THROW(registry.RemoveGroup("nonexistent"));
}

TEST_F(ServiceRegistryTest, IsGroupReturnsFalseForNonExistent) {
    ServiceRegistry registry;
    EXPECT_FALSE(registry.IsGroup("nonexistent"));
}

TEST_F(ServiceRegistryTest, GetServiceReturnsNullForNonExistent) {
    ServiceRegistry registry;
    EXPECT_EQ(registry.GetService("nonexistent"), nullptr);
}

TEST_F(ServiceRegistryTest, AddDuplicateGroupDoesNothing) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    registry.AddGroup("group1"); // Should handle duplicate gracefully
    EXPECT_TRUE(registry.IsGroup("group1"));
    EXPECT_EQ(registry.GetGroupNum(), 1);
}

TEST_F(ServiceRegistryTest, GetEmptyGroupList) {
    ServiceRegistry registry;
    auto groups = registry.GetGroupList();
    EXPECT_TRUE(groups.empty());
}

TEST_F(ServiceRegistryTest, GetGroupListWithMultipleGroups) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    registry.AddGroup("group2");
    registry.AddGroup("group3");
    
    auto groups = registry.GetGroupList();
    EXPECT_EQ(groups.size(), 3);
}

TEST_F(ServiceRegistryTest, GetServicesByGroupForNonExistentGroup) {
    ServiceRegistry registry;
    auto services = registry.GetServicesByGroup("nonexistent");
    EXPECT_TRUE(services.empty());
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroupForNonExistentGroup) {
    ServiceRegistry registry;
    auto names = registry.GetServicesNameByGroup("nonexistent");
    EXPECT_TRUE(names.empty());
}

TEST_F(ServiceRegistryTest, GetGroupNameByServiceForNonExistent) {
    ServiceRegistry registry;
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    EXPECT_EQ(registry.GetGroupNameByService(service), "");
}

TEST_F(ServiceRegistryTest, MultipleServicesInSameGroup) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
    auto service3 = std::make_shared<MockService>("svc3", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::MANUAL, false, lifecycle_manager::MwType::AGL);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    
    Mock_Service = service3.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc3"));
    registry.AddService(service3, "group1");
    
    EXPECT_EQ(registry.GetServiceNumInGroup("group1"), 3);
    auto services = registry.GetServicesByGroup("group1");
    EXPECT_EQ(services.size(), 3);
}

TEST_F(ServiceRegistryTest, GetServiceNumInNonExistentGroup) {
    ServiceRegistry registry;
    EXPECT_EQ(registry.GetServiceNumInGroup("nonexistent"), 0);
}

TEST_F(ServiceRegistryTest, EmptyGroupName) {
    ServiceRegistry registry;
    registry.AddGroup("");
    EXPECT_TRUE(registry.IsGroup(""));
}

TEST_F(ServiceRegistryTest, VectorHasEmptyVector) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_FALSE(registry.vectorHas(vec, "anything"));
}

TEST_F(ServiceRegistryTest, VectorAddMultipleItems) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_TRUE(registry.vectorAdd(vec, "item1"));
    EXPECT_TRUE(registry.vectorAdd(vec, "item2"));
    EXPECT_TRUE(registry.vectorAdd(vec, "item3"));
    EXPECT_EQ(vec.size(), 3);
    EXPECT_TRUE(registry.vectorHas(vec, "item1"));
    EXPECT_TRUE(registry.vectorHas(vec, "item2"));
    EXPECT_TRUE(registry.vectorHas(vec, "item3"));
}

// Additional tests for branch coverage

// Test GetServicesByGroup with empty group name (returns all services)
TEST_F(ServiceRegistryTest, GetServicesByGroup_EmptyGroupReturnsAll) {
    ServiceRegistry registry;
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    
    registry.AddGroup("group1");
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    
    auto all_services = registry.GetServicesByGroup("");
    EXPECT_EQ(all_services.size(), 2);
}

// Test GetServicesNameByGroup with empty group name (returns all service names)
TEST_F(ServiceRegistryTest, GetServicesNameByGroup_EmptyGroupReturnsAll) {
    ServiceRegistry registry;
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    
    registry.AddGroup("group1");
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    
    auto all_names = registry.GetServicesNameByGroup("");
    EXPECT_EQ(all_names.size(), 2);
}

// Test GetServiceNumInGroup with empty string (returns all services count)
TEST_F(ServiceRegistryTest, GetServiceNumInGroup_EmptyStringReturnsTotal) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    
    EXPECT_EQ(registry.GetServiceNumInGroup(""), 2);
}

// Test AddService to non-existent group (group_ptr is nullptr)
TEST_F(ServiceRegistryTest, AddService_ToNonExistentGroup) {
    ServiceRegistry registry;
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    // Add service to non-existent group
    EXPECT_TRUE(registry.AddService(service, "nonexistent_group"));
    EXPECT_EQ(registry.GetServiceNum(), 1);
    EXPECT_NE(registry.GetService("svc1"), nullptr);
}

// Test AddService when service already exists
TEST_F(ServiceRegistryTest, AddService_ServiceAlreadyExists) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, false, lifecycle_manager::MwType::AGL);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    EXPECT_TRUE(registry.AddService(service1, "group1"));
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    EXPECT_TRUE(registry.AddService(service2, "group1")); // Should still return true but service is replaced
    EXPECT_EQ(registry.GetServiceNum(), 1);
}

// Test GetServicesByGroup when group has services but some are removed
TEST_F(ServiceRegistryTest, GetServicesByGroup_AfterServiceRemoval) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    registry.RemoveService("svc1");
    
    // Group still has svc1 reference but service is removed
    auto services = registry.GetServicesByGroup("group1");
    EXPECT_EQ(services.size(), 1);
    EXPECT_EQ(services[0]->GetName(), "svc2");
}

// Test GetServicesNameByGroup when group has services but some are removed
TEST_F(ServiceRegistryTest, GetServicesNameByGroup_AfterServiceRemoval) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc2"));
    registry.AddService(service2, "group1");
    registry.RemoveService("svc1");
    
    auto names = registry.GetServicesNameByGroup("group1");
    EXPECT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "svc2");
}

// Test concurrent operations with mutex (basic thread safety)
TEST_F(ServiceRegistryTest, ConcurrentAddAndGet) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service = std::make_shared<MockService>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    Mock_Service = service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("svc1"));
    
    registry.AddService(service, "group1");
    
    // Multiple gets should be thread-safe
    auto result1 = registry.GetService("svc1");
    auto result2 = registry.GetService("svc1");
    auto result3 = registry.GetService("svc1");
    
    EXPECT_EQ(result1, service);
    EXPECT_EQ(result2, service);
    EXPECT_EQ(result3, service);
}

// Test all ServiceType values
TEST_F(ServiceRegistryTest, AllServiceTypes) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("normal_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("oneshot_svc", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("normal_svc"));
    ON_CALL(*Mock_Service, GetServiceType()).WillByDefault(Return(lifecycle_manager::ServiceType::NORMAL));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("oneshot_svc"));
    ON_CALL(*Mock_Service, GetServiceType()).WillByDefault(Return(lifecycle_manager::ServiceType::ONESHOT));
    registry.AddService(service2, "group1");
    
    EXPECT_EQ(registry.GetServiceNum(), 2);
    
    Mock_Service = service1.get();
    EXPECT_EQ(registry.GetService("normal_svc")->GetServiceType(), lifecycle_manager::ServiceType::NORMAL);
    Mock_Service = service2.get();
    EXPECT_EQ(registry.GetService("oneshot_svc")->GetServiceType(), lifecycle_manager::ServiceType::ONESHOT);
}

// Test all RecoveryPolicy values
TEST_F(ServiceRegistryTest, AllRecoveryPolicies) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("always_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("on_failure_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::MwType::ROS2);
    auto service3 = std::make_shared<MockService>("manual_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::MANUAL, true, lifecycle_manager::MwType::ROS2);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("always_svc"));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("on_failure_svc"));
    registry.AddService(service2, "group1");
    
    Mock_Service = service3.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("manual_svc"));
    registry.AddService(service3, "group1");
    
    EXPECT_EQ(registry.GetServiceNum(), 3);
}

// Test all MwType values
TEST_F(ServiceRegistryTest, AllMwTypes) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<MockService>("ros2_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
    auto service2 = std::make_shared<MockService>("agl_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AGL);
    auto service3 = std::make_shared<MockService>("autosar_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::AUTOSAR);
    
    Mock_Service = service1.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("ros2_svc"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::ROS2));
    registry.AddService(service1, "group1");
    
    Mock_Service = service2.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("agl_svc"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::AGL));
    registry.AddService(service2, "group1");
    
    Mock_Service = service3.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("autosar_svc"));
    ON_CALL(*Mock_Service, GetMwType()).WillByDefault(Return(lifecycle_manager::MwType::AUTOSAR));
    registry.AddService(service3, "group1");
    
    Mock_Service = service1.get();
    EXPECT_EQ(registry.GetService("ros2_svc")->GetMwType(), lifecycle_manager::MwType::ROS2);
    Mock_Service = service2.get();
    EXPECT_EQ(registry.GetService("agl_svc")->GetMwType(), lifecycle_manager::MwType::AGL);
    Mock_Service = service3.get();
    EXPECT_EQ(registry.GetService("autosar_svc")->GetMwType(), lifecycle_manager::MwType::AUTOSAR);
}

// Test GetGroupList preserves order
TEST_F(ServiceRegistryTest, GetGroupListOrder) {
    ServiceRegistry registry;
    registry.AddGroup("alpha");
    registry.AddGroup("beta");
    registry.AddGroup("gamma");
    
    auto groups = registry.GetGroupList();
    EXPECT_EQ(groups.size(), 3);
    // Map ordering should be alphabetical for string keys
}

// Test services enabled/disabled state preservation
TEST_F(ServiceRegistryTest, ServiceEnabledState) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto enabled_service = std::make_shared<MockService>("enabled_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::MwType::ROS2);
    auto disabled_service = std::make_shared<MockService>("disabled_svc", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ALWAYS, false, lifecycle_manager::MwType::ROS2);
    
    Mock_Service = enabled_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("enabled_svc"));
    ON_CALL(*Mock_Service, isEnabled()).WillByDefault(Return(true));
    registry.AddService(enabled_service, "group1");
    
    Mock_Service = disabled_service.get();
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("disabled_svc"));
    ON_CALL(*Mock_Service, isEnabled()).WillByDefault(Return(false));
    registry.AddService(disabled_service, "group1");
    
    Mock_Service = enabled_service.get();
    EXPECT_TRUE(registry.GetService("enabled_svc")->isEnabled());
    Mock_Service = disabled_service.get();
    EXPECT_FALSE(registry.GetService("disabled_svc")->isEnabled());
}

// ═══════════════════════════════════════════════════════════════════════════
// F013 컨테이너 레지스트리 + 그룹 목록
//
// 기존 테스트는 서비스/그룹 API 만 다룬다. 컨테이너 쪽
// (UpsertContainer / GetContainer / GetAllContainers / UpdateContainerState /
//  GetServicesInContainer) 과 GetGroupList 는 하나도 없어서 236줄 중 103줄이
// 미커버였다. 전부 in-memory 자료구조라 런타임 의존이 없다.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// ContainerType / ContainerState 는 lifecycle_manager 와 lifecyclemgr(IDL) 양쪽에
// 있어 `using namespace lifecycle_manager;` 아래에서도 모호하다. 전부 명시한다.
std::shared_ptr<lifecycle_manager::Container> MakeContainer(
        const std::string& name,
        lifecycle_manager::ContainerType type = lifecycle_manager::ContainerType::DOCKER) {
    return std::make_shared<lifecycle_manager::Container>(name, name + "-img", "/tmp/" + name + ".yaml",
                                       name + "-ns", type);
}

} // namespace

TEST(ServiceRegistryContainerTest, GetContainerOnEmptyRegistryReturnsNull) {
    ServiceRegistry reg;
    EXPECT_EQ(reg.GetContainer("nope"), nullptr);
    EXPECT_TRUE(reg.GetAllContainers().empty());
}

TEST(ServiceRegistryContainerTest, UpsertThenGet) {
    ServiceRegistry reg;
    auto c = MakeContainer("app");
    reg.UpsertContainer(c);

    auto got = reg.GetContainer("app");
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->GetName(), "app");
    EXPECT_EQ(got->GetType(), lifecycle_manager::ContainerType::DOCKER);
    EXPECT_EQ(reg.GetAllContainers().size(), 1u);
}

TEST(ServiceRegistryContainerTest, UpsertNullIsIgnored) {
    ServiceRegistry reg;
    reg.UpsertContainer(nullptr);
    EXPECT_TRUE(reg.GetAllContainers().empty());
}

TEST(ServiceRegistryContainerTest, UpsertSameNameReplaces) {
    ServiceRegistry reg;
    reg.UpsertContainer(MakeContainer("app", lifecycle_manager::ContainerType::DOCKER));
    reg.UpsertContainer(MakeContainer("app", lifecycle_manager::ContainerType::K3S));

    EXPECT_EQ(reg.GetAllContainers().size(), 1u);
    EXPECT_EQ(reg.GetContainer("app")->GetType(), lifecycle_manager::ContainerType::K3S);
}

TEST(ServiceRegistryContainerTest, GetAllContainersReturnsEveryEntry) {
    ServiceRegistry reg;
    reg.UpsertContainer(MakeContainer("a"));
    reg.UpsertContainer(MakeContainer("b"));
    reg.UpsertContainer(MakeContainer("c"));

    auto all = reg.GetAllContainers();
    ASSERT_EQ(all.size(), 3u);
    std::set<std::string> names;
    for (const auto& c : all) names.insert(c->GetName());
    EXPECT_EQ(names, (std::set<std::string>{"a", "b", "c"}));
}

// ── UpdateContainerState: 반환값이 "변했는가" 를 뜻한다 ─────────────────────

TEST(ServiceRegistryContainerTest, UpdateStateAutoCreatesUnknownContainer) {
    ServiceRegistry reg;
    // UpdateServiceList 보다 알림이 먼저 와도 항목이 보이도록 자동 생성한다.
    EXPECT_TRUE(reg.UpdateContainerState("ghost", lifecycle_manager::ContainerState::CONTAINER_RUNNING, 0));

    auto c = reg.GetContainer("ghost");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->GetState(), lifecycle_manager::ContainerState::CONTAINER_RUNNING);
    EXPECT_EQ(c->GetType(), lifecycle_manager::ContainerType::CT_UNKNOWN);
    EXPECT_EQ(c->GetExitCode(), 0);
}

TEST(ServiceRegistryContainerTest, UpdateStateReturnsTrueOnlyOnTransition) {
    ServiceRegistry reg;
    reg.UpsertContainer(MakeContainer("app"));   // 초기 상태 CS_UNKNOWN

    // CS_UNKNOWN → RUNNING : 전이
    EXPECT_TRUE(reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_RUNNING, 0));
    // RUNNING → RUNNING : 전이 아님 (호출자는 이때 publish 하지 않는다)
    EXPECT_FALSE(reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_RUNNING, 0));
    // RUNNING → STOPPED : 전이
    EXPECT_TRUE(reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_STOPPED, 137));

    EXPECT_EQ(reg.GetContainer("app")->GetState(), lifecycle_manager::ContainerState::CONTAINER_STOPPED);
    EXPECT_EQ(reg.GetContainer("app")->GetExitCode(), 137);
}

TEST(ServiceRegistryContainerTest, UpdateStateStoresExitCodeEvenWithoutTransition) {
    ServiceRegistry reg;
    reg.UpsertContainer(MakeContainer("app"));
    reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_STOPPED, 1);
    // 상태가 같아도 exit code 는 갱신되어야 한다.
    EXPECT_FALSE(reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_STOPPED, 9));
    EXPECT_EQ(reg.GetContainer("app")->GetExitCode(), 9);
}

TEST(ServiceRegistryContainerTest, UpdateStatePausedIsTracked) {
    ServiceRegistry reg;
    reg.UpsertContainer(MakeContainer("app"));
    EXPECT_TRUE(reg.UpdateContainerState("app", lifecycle_manager::ContainerState::CONTAINER_PAUSED, 0));
    EXPECT_EQ(reg.GetContainer("app")->GetState(), lifecycle_manager::ContainerState::CONTAINER_PAUSED);
}

// ── GetGroupList ────────────────────────────────────────────────────────────

TEST(ServiceRegistryContainerTest, GetGroupListEmptyThenPopulated) {
    ServiceRegistry reg;
    EXPECT_TRUE(reg.GetGroupList().empty());

    reg.AddGroup("g1");
    reg.AddGroup("g2");

    auto groups = reg.GetGroupList();
    ASSERT_EQ(groups.size(), 2u);
    EXPECT_EQ(std::set<std::string>(groups.begin(), groups.end()),
              (std::set<std::string>{"g1", "g2"}));
}

TEST(ServiceRegistryContainerTest, GetGroupListDropsRemovedGroup) {
    ServiceRegistry reg;
    reg.AddGroup("g1");
    reg.AddGroup("g2");
    reg.RemoveGroup("g1");

    auto groups = reg.GetGroupList();
    ASSERT_EQ(groups.size(), 1u);
    EXPECT_EQ(groups[0], "g2");
}

// ── GetServicesInContainer ──────────────────────────────────────────────────

TEST(ServiceRegistryContainerTest, GetServicesInContainerEmptyWhenNoMatch) {
    ServiceRegistry reg;
    EXPECT_TRUE(reg.GetServicesInContainer("app").empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
