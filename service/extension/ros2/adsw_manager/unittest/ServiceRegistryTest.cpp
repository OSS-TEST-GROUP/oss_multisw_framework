#include <gtest/gtest.h>
#include "service/ServiceRegistry.cpp"  // Include implementation directl
#include "service/Service.cpp"
#include "logger/Logger.cpp"

using namespace ros2::adsw_manager;
using namespace lifecycle_manager;

class ServiceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_unique<ServiceRegistry>();
    }

    void TearDown() override {
        registry.reset();
    }

    std::shared_ptr<Service> MakeService(const std::string& name) {
        return std::make_shared<Service>(name, ServiceType::ONESHOT, RecoveryPolicy::ALWAYS, true);
    }

    std::unique_ptr<ServiceRegistry> registry;
};

TEST_F(ServiceRegistryTest, AddAndGetService_TC001) {
    auto service = MakeService("MyService");
    EXPECT_TRUE(registry->AddService(service, "GroupA"));

    auto retrieved = registry->GetService("MyService");
    EXPECT_EQ(retrieved, service);
}

TEST_F(ServiceRegistryTest, AddDuplicateService_TC002) {
    auto service1 = MakeService("DuplicateService");
    auto service2 = MakeService("DuplicateService");

    EXPECT_TRUE(registry->AddService(service1, "GroupA"));
    EXPECT_FALSE(registry->AddService(service2, "GroupA"));  // Should be rejected
}

TEST_F(ServiceRegistryTest, RemoveService_TC003) {
    auto service = MakeService("ToBeRemoved");
    registry->AddService(service, "GroupA");

    registry->RemoveService("ToBeRemoved");

    EXPECT_EQ(registry->GetService("ToBeRemoved"), nullptr);
    EXPECT_EQ(registry->GetServiceNumInGroup("GroupA"), 0);
}

TEST_F(ServiceRegistryTest, GroupHandling_TC004) {
    registry->AddGroup("NewGroup");
    EXPECT_TRUE(registry->IsGroup("NewGroup"));

    registry->RemoveGroup("NewGroup");
    EXPECT_FALSE(registry->IsGroup("NewGroup"));
}

TEST_F(ServiceRegistryTest, GetServiceByGroup_TC005) {
    auto service1 = MakeService("S1");
    auto service2 = MakeService("S2");

    registry->AddService(service1, "Sensors");
    registry->AddService(service2, "Sensors");

    auto list = registry->GetServicesByGroup("Sensors");
    EXPECT_EQ(list.size(), 2);
    EXPECT_TRUE(std::find(list.begin(), list.end(), service1) != list.end());
    EXPECT_TRUE(std::find(list.begin(), list.end(), service2) != list.end());
}

TEST_F(ServiceRegistryTest, GetGroupNameByService_TC006) {
    auto service = MakeService("GroupedService");
    registry->AddService(service, "MyGroup");

    EXPECT_EQ(registry->GetGroupNameByService(service), "MyGroup");
}

TEST_F(ServiceRegistryTest, ServiceCounts_TC007) {
    EXPECT_EQ(registry->GetServicesNum(), 0);
    EXPECT_EQ(registry->GetGroupNum(), 0);

    auto s1 = MakeService("A");
    auto s2 = MakeService("B");

    registry->AddService(s1, "G1");
    registry->AddService(s2, "G2");

    EXPECT_EQ(registry->GetServicesNum(), 2);
    EXPECT_EQ(registry->GetGroupNum(), 2);
    EXPECT_EQ(registry->GetServiceNumInGroup("G1"), 1);
    EXPECT_EQ(registry->GetServiceNumInGroup("G2"), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
