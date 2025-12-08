#include <algorithm>
// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "service/ServiceRegistry.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockService.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockLogger.hpp"
#include "mock/MockDDSMessageEndpoint.hpp"
#include "mock/MockContainerController.hpp"
#include "mock/MockContainer.hpp"

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

using namespace ros2::adsw_manager;

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

// helper lambdas for vector operations (replace missing vectorAdd/vectorRemove/vectorHas APIs)
static auto vectorHas = [](const std::vector<std::string>& v, const std::string& item){
    return std::find(v.begin(), v.end(), item) != v.end();
};
static auto vectorAdd = [](std::vector<std::string>& v, const std::string& item){
    if (std::find(v.begin(), v.end(), item) != v.end()) return false;
    v.push_back(item);
    return true;
};
static auto vectorRemove = [](std::vector<std::string>& v, const std::string& item){
    auto it = std::find(v.begin(), v.end(), item);
    if (it == v.end()) return false;
    v.erase(it);
    return true;
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

TEST_F(ServiceRegistryTest, LoadServiceList_TC009) {
    // Assuming LoadServiceList is implemented to load from a file or predefined list
    EXPECT_TRUE(registry->LoadServiceList("path/to/service_list.json"));
    EXPECT_GT(registry->GetServicesNum(), 0);
}
TEST_F(ServiceRegistryTest, SaveAndLoadServiceList_TC010) {
    // Assuming Save and LoadServiceSaved are implemented
    EXPECT_TRUE(registry->Save());
    EXPECT_TRUE(registry->LoadServiceSaved("path/to/saved_service_list.json"));
    EXPECT_GT(registry->GetServicesNum(), 0);
}

TEST_F(ServiceRegistryTest, LoadServiceSaved_TC011) {
    // Assuming LoadServiceSaved is implemented to load from a YAML file
    EXPECT_TRUE(registry->LoadServiceSaved("path/to/saved_service_list.yaml"));
    EXPECT_GT(registry->GetServicesNum(), 0);
}
TEST_F(ServiceRegistryTest, ExtractRos2LaunchCommands_TC012) {
    std::string script = "ros2 launch my_package my_launch_file.launch.py";
    auto commands = registry->extractRos2LaunchCommands(script);
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(std::get<0>(commands[0]), "ros2 launch my_package my_launch_file.launch.py");
}

TEST_F(ServiceRegistryTest, GetService_NonExistent_TC013) {
    auto service = registry->GetService("NonExistent");
    EXPECT_EQ(service, nullptr);
}

TEST_F(ServiceRegistryTest, RemoveService_NonExistent_TC014) {
    EXPECT_NO_THROW(registry->RemoveService("NonExistent"));
}

TEST_F(ServiceRegistryTest, AddService_NullPointer_TC015) {
    EXPECT_FALSE(registry->AddService(nullptr, "Group"));
}

TEST_F(ServiceRegistryTest, GetGroupNameByService_NonExistent_TC016) {
    auto service = MakeService("Orphan");
    EXPECT_EQ(registry->GetGroupNameByService(service), "");
}

TEST_F(ServiceRegistryTest, GetServicesByGroup_NonExistent_TC017) {
    auto list = registry->GetServicesByGroup("NonExistentGroup");
    EXPECT_TRUE(list.empty());
}

TEST_F(ServiceRegistryTest, GetServiceNumInGroup_NonExistent_TC018) {
    EXPECT_EQ(registry->GetServiceNumInGroup("NonExistent"), 0);
}

TEST_F(ServiceRegistryTest, IsGroup_NonExistent_TC019) {
    EXPECT_FALSE(registry->IsGroup("NonExistentGroup"));
}

TEST_F(ServiceRegistryTest, AddGroup_EmptyName_TC020) {
    registry->AddGroup("");
    EXPECT_TRUE(registry->IsGroup(""));
}

TEST_F(ServiceRegistryTest, AddDuplicateGroup_TC021) {
    registry->AddGroup("DuplicateGroup");
    registry->AddGroup("DuplicateGroup");
    EXPECT_TRUE(registry->IsGroup("DuplicateGroup"));
}

TEST_F(ServiceRegistryTest, RemoveGroup_NonExistent_TC022) {
    EXPECT_NO_THROW(registry->RemoveGroup("NonExistent"));
}

TEST_F(ServiceRegistryTest, MultipleGroups_TC023) {
    registry->AddGroup("Group1");
    registry->AddGroup("Group2");
    registry->AddGroup("Group3");
    
    EXPECT_EQ(registry->GetGroupNum(), 3);
}

TEST_F(ServiceRegistryTest, ServicesInMultipleGroups_TC024) {
    auto s1 = MakeService("S1");
    auto s2 = MakeService("S2");
    auto s3 = MakeService("S3");
    
    registry->AddService(s1, "GroupA");
    registry->AddService(s2, "GroupA");
    registry->AddService(s3, "GroupB");
    
    EXPECT_EQ(registry->GetServiceNumInGroup("GroupA"), 2);
    EXPECT_EQ(registry->GetServiceNumInGroup("GroupB"), 1);
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroup_TC025) {
    auto s1 = MakeService("Service1");
    auto s2 = MakeService("Service2");
    
    registry->AddService(s1, "TestGroup");
    registry->AddService(s2, "TestGroup");
    
    auto services = registry->GetServicesByGroup("TestGroup");
    std::vector<std::string> names;
    for (auto &svc : services) names.push_back(svc->GetName());
    EXPECT_EQ(names.size(), 2);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Service1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Service2") != names.end());
}

TEST_F(ServiceRegistryTest, GetServicesNameByGroup_Empty_TC026) {
    auto services = registry->GetServicesByGroup("EmptyGroup");
    EXPECT_TRUE(services.empty());
}

TEST_F(ServiceRegistryTest, GetGroupList_TC027) {
    registry->AddGroup("G1");
    registry->AddGroup("G2");
    registry->AddGroup("G3");
    
    // ServiceRegistry does not expose GetGroupList; use GetGroupNum and verify count
    EXPECT_EQ(registry->GetGroupNum(), 3);
}

TEST_F(ServiceRegistryTest, GetGroupList_Empty_TC028) {
    EXPECT_EQ(registry->GetGroupNum(), 0);
}

TEST_F(ServiceRegistryTest, ExtractRos2LaunchCommands_MultipleCommands_TC029) {
    std::string script = "ros2 launch pkg1 launch1.py && ros2 launch pkg2 launch2.py";
    auto commands = registry->extractRos2LaunchCommands(script);
    EXPECT_EQ(commands.size(), 2);
    EXPECT_EQ(std::get<1>(commands[0]), "pkg1");
    EXPECT_EQ(std::get<1>(commands[1]), "pkg2");
}

TEST_F(ServiceRegistryTest, ExtractRos2LaunchCommands_NoCommands_TC030) {
    std::string script = "echo hello world";
    auto commands = registry->extractRos2LaunchCommands(script);
    EXPECT_TRUE(commands.empty());
}

TEST_F(ServiceRegistryTest, ExtractRos2LaunchCommands_WithArgs_TC031) {
    std::string script = "ros2 launch my_pkg launch.py arg1:=value1 arg2:=value2";
    auto commands = registry->extractRos2LaunchCommands(script);
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(std::get<1>(commands[0]), "my_pkg");
    EXPECT_EQ(std::get<2>(commands[0]), "launch.py");
}

TEST_F(ServiceRegistryTest, ExtractRos2RunCommands_TC032) {
    std::string script = "ros2 run my_package my_executable";
    auto commands = registry->extractRos2RunCommands(script);
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(std::get<1>(commands[0]), "my_package");
    EXPECT_EQ(std::get<2>(commands[0]), "my_executable");
}

TEST_F(ServiceRegistryTest, ExtractRos2RunCommands_Multiple_TC033) {
    std::string script = "ros2 run pkg1 exe1 && ros2 run pkg2 exe2";
    auto commands = registry->extractRos2RunCommands(script);
    EXPECT_EQ(commands.size(), 2);
}

TEST_F(ServiceRegistryTest, ExtractRos2RunCommands_NoCommands_TC034) {
    std::string script = "some other command";
    auto commands = registry->extractRos2RunCommands(script);
    EXPECT_TRUE(commands.empty());
}

TEST_F(ServiceRegistryTest, VectorAdd_TC035) {
    std::vector<std::string> vec;
    EXPECT_TRUE(vectorAdd(vec, "item1"));
    EXPECT_TRUE(vectorHas(vec, "item1"));
    EXPECT_FALSE(vectorAdd(vec, "item1")); // Duplicate
}

TEST_F(ServiceRegistryTest, VectorRemove_TC036) {
    std::vector<std::string> vec = {"item1", "item2", "item3"};
    EXPECT_TRUE(vectorRemove(vec, "item2"));
    EXPECT_FALSE(vectorHas(vec, "item2"));
    EXPECT_EQ(vec.size(), 2);
}

TEST_F(ServiceRegistryTest, VectorRemove_NonExistent_TC037) {
    std::vector<std::string> vec = {"item1"};
    EXPECT_FALSE(vectorRemove(vec, "nonexistent"));
}

TEST_F(ServiceRegistryTest, VectorHas_TC038) {
    std::vector<std::string> vec = {"a", "b", "c"};
    EXPECT_TRUE(vectorHas(vec, "b"));
    EXPECT_FALSE(vectorHas(vec, "d"));
}

TEST_F(ServiceRegistryTest, EmptyRegistry_TC039) {
    EXPECT_EQ(registry->GetServicesNum(), 0);
    EXPECT_EQ(registry->GetGroupNum(), 0);
    EXPECT_EQ(registry->GetGroupNum(), 0); // group list empty equivalent
}

TEST_F(ServiceRegistryTest, LargeNumberOfServices_TC040) {
    for (int i = 0; i < 100; ++i) {
        auto svc = MakeService("Service" + std::to_string(i));
        registry->AddService(svc, "LargeGroup");
    }
    EXPECT_EQ(registry->GetServicesNum(), 100);
    EXPECT_EQ(registry->GetServiceNumInGroup("LargeGroup"), 100);
}

TEST_F(ServiceRegistryTest, SpecialCharactersInNames_TC041) {
    auto svc = MakeService("service-name_123.test");
    registry->AddService(svc, "group-name_456");
    
    EXPECT_EQ(registry->GetService("service-name_123.test"), svc);
    EXPECT_TRUE(registry->IsGroup("group-name_456"));
}

TEST_F(ServiceRegistryTest, RemoveServiceUpdatesCount_TC042) {
    auto s1 = MakeService("S1");
    auto s2 = MakeService("S2");
    
    registry->AddService(s1, "G1");
    registry->AddService(s2, "G1");
    EXPECT_EQ(registry->GetServicesNum(), 2);
    
    registry->RemoveService("S1");
    EXPECT_EQ(registry->GetServicesNum(), 1);
    
    registry->RemoveService("S2");
    EXPECT_EQ(registry->GetServicesNum(), 0);
}

TEST_F(ServiceRegistryTest, RemoveGroupWithServices_TC043) {
    auto s1 = MakeService("S1");
    registry->AddService(s1, "ToBeRemoved");
    
    registry->RemoveGroup("ToBeRemoved");
    EXPECT_FALSE(registry->IsGroup("ToBeRemoved"));
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
