// Unit tests for ServiceRegistry class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "service/ServiceRegistry.hpp"
#include "unittest/mock/mock_service_registry.hpp"
#include "unittest/mock/mock_service.hpp"

#include "service/Service.hpp"

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

class ServiceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(ServiceRegistryTest, GetServiceReturnsExpected) {
    MockServiceRegistry registry;
    auto expected_service = std::make_shared<Service>();
    EXPECT_CALL(registry, GetService("test")).WillOnce(::testing::Return(expected_service));
    auto result = registry.GetService("test");
    EXPECT_EQ(result, expected_service);
}

// Add more tests for ServiceRegistry as needed
TEST_F(ServiceRegistryTest, AddAndGetService) {
    ServiceRegistry registry;
    auto service = std::make_shared<Service>("svc1", lifecycle_manager::ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true);
    EXPECT_TRUE(registry.AddService(service, ""));
    auto fetched = registry.GetService("svc1");
    EXPECT_EQ(fetched, service);
}

TEST_F(ServiceRegistryTest, AddDuplicateServiceReturnsTrueButDoesNotDuplicate) {
    ServiceRegistry registry;
    auto service = std::make_shared<Service>("svc1", lifecycle_manager:ServiceType::NORMAL, lifecycle_manager::RecoveryPolicy::ON_FAILURE, true);
    EXPECT_TRUE(registry.AddService(service, ""));
    EXPECT_TRUE(registry.AddService(service, "")); // No duplicate check in AddService
    EXPECT_EQ(registry.GetServiceNum(), 1);
}

TEST_F(ServiceRegistryTest, RemoveServiceRemovesIt) {
    ServiceRegistry registry;
    auto service = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service, "");
    registry.RemoveService("svc1");
    EXPECT_EQ(registry.GetService("svc1"), nullptr);
}

TEST_F(ServiceRegistryTest, AddAndRemoveGroup) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    EXPECT_TRUE(registry.IsGroup("group1"));
    registry.RemoveGroup("group1");
    EXPECT_FALSE(registry.IsGroup("group1"));
}

TEST_F(ServiceRegistryTest, GetServicesByGroupReturnsCorrectServices) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    auto service1 = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    auto service2 = std::make_shared<Service>("svc2", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service1, "group1");
    registry.AddService(service2, "");
    auto group_services = registry.GetServicesByGroup("group1");
    ASSERT_EQ(group_services.size(), 1);
    EXPECT_EQ(group_services[0]->GetName(), "svc1");
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroupReturnsCorrectNames) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    auto service1 = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service1, "group1");
    auto names = registry.GetServicesNameByGroup("group1");
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "svc1");
}

TEST_F(ServiceRegistryTest, GetGroupNameByServiceReturnsCorrectGroup) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    auto service1 = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service1, "group1");
    EXPECT_EQ(registry.GetGroupNameByService(service1), "group1");
}

TEST_F(ServiceRegistryTest, GetServiceNumAndGroupNum) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    auto service1 = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service1, "group1");
    EXPECT_EQ(registry.GetServiceNum(), 1);
    EXPECT_EQ(registry.GetGroupNum(), 1);
}

TEST_F(ServiceRegistryTest, GetServiceNumInGroup) {
    ServiceRegistry registry;
    registry.AddGroup("group1");
    auto service1 = std::make_shared<Service>("svc1", ServiceType::NORMAL, RecoveryPolicy::ON_FAILURE, true);
    registry.AddService(service1, "group1");
    EXPECT_EQ(registry.GetServiceNumInGroup("group1"), 1);
    EXPECT_EQ(registry.GetServiceNumInGroup(""), 1);
}

TEST_F(ServiceRegistryTest, VectorHasAddRemove) {
    ServiceRegistry registry;
    std::vector<std::string> vec;
    EXPECT_TRUE(registry.vectorAdd(vec, "item1"));
    EXPECT_TRUE(registry.vectorHas(vec, "item1"));
    EXPECT_TRUE(registry.vectorRemove(vec, "item1"));
    EXPECT_FALSE(registry.vectorHas(vec, "item1"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
