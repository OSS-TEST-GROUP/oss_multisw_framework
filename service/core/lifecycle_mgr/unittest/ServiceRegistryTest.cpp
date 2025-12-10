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
    }
    virtual void TearDown(){
        delete ins;
    }

    ServiceRegistry* ins;
};

TEST_F(ServiceRegistryTest, GetServiceReturnsExpected) {
    auto expected_service = std::make_shared<MockService>("test", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    ins->AddService(expected_service, "");
    auto result = ins->GetService("test");
    EXPECT_EQ(result, expected_service);
}

// Add more tests for ServiceRegistry as needed
TEST_F(ServiceRegistryTest, AddAndGetService) {
    auto service = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    EXPECT_TRUE(ins->AddService(service, ""));
    auto fetched = ins->GetService("svc1");
    EXPECT_EQ(fetched, service);
}

TEST_F(ServiceRegistryTest, AddDuplicateServiceReturnsTrueButDoesNotDuplicate) {
    auto service = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    EXPECT_TRUE(ins->AddService(service, ""));
    EXPECT_TRUE(ins->AddService(service, "")); // No duplicate check in AddService
    EXPECT_EQ(ins->GetServiceNum(), 1);
}

TEST_F(ServiceRegistryTest, RemoveServiceRemovesIt) {
    auto service = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
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
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    auto service2 = std::make_shared<lifecycle_manager::Service>("svc2", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    ins->AddService(service1, "group1");
    ins->AddService(service2, "");
    auto group_services = ins->GetServicesByGroup("group1");
    ASSERT_EQ(group_services.size(), 1);
    EXPECT_EQ(group_services[0]->GetName(), "svc1");
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroupReturnsCorrectNames) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    ins->AddService(service1, "group1");
    auto names = ins->GetServicesNameByGroup("group1");
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "svc1");
}

TEST_F(ServiceRegistryTest, GetGroupNameByServiceReturnsCorrectGroup) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    ins->AddService(service1, "group1");
    EXPECT_EQ(ins->GetGroupNameByService(service1), "group1");
}

TEST_F(ServiceRegistryTest, GetServiceNumAndGroupNum) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    ins->AddService(service1, "group1");
    EXPECT_EQ(ins->GetServiceNum(), 1);
    EXPECT_EQ(ins->GetGroupNum(), 1);
}

TEST_F(ServiceRegistryTest, GetServiceNumInGroup) {
    ins->AddGroup("group1");
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
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
    
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    
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
    auto service = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    EXPECT_EQ(registry.GetGroupNameByService(service), "");
}

TEST_F(ServiceRegistryTest, MultipleServicesInSameGroup) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    
    auto service1 = std::make_shared<lifecycle_manager::Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true, lifecycle_manager::OsType::ROS2);
    auto service2 = std::make_shared<lifecycle_manager::Service>("svc2", lifecycle_manager::ServiceType::ONESHOT, lifecycle_manager::RecoveryPolicy::ALWAYS, true, lifecycle_manager::OsType::ROS2);
    auto service3 = std::make_shared<lifecycle_manager::Service>("svc3", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::MANUAL, false, lifecycle_manager::OsType::AGL);
    
    registry.AddService(service1, "group1");
    registry.AddService(service2, "group1");
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
