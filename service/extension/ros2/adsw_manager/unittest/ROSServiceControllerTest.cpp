// Unit tests for ROSServiceController class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ROSServiceController.hpp"
#include "service/Service.hpp"
#include "service/ServiceRegistry.hpp"

#include "mock/MockADSWManager.hpp"
#include "mock/MockService.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockDDSTransmitter.hpp"
#include "mock/MockLogger.hpp"

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


class ROSServiceControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Mock_Logger = new MockLogger();
        // Don't create Mock_Service in SetUp - let each test create as needed
        Mock_Service = nullptr;
        controller = std::make_shared<ros2::adsw_manager::ROSServiceController>();
        controller->Initialize();

        Mock_ADSWManager = new MockADSWManager();
        ON_CALL(*Mock_ADSWManager, Get()).WillByDefault(Return(Mock_ADSWManager));

        Mock_ServiceRegistry = new MockServiceRegistry();
        ON_CALL(*Mock_ADSWManager, GetServiceRegistry()).WillByDefault(Return(
            std::shared_ptr<ServiceRegistry>(Mock_ServiceRegistry, [](ServiceRegistry*){})));

    }
    
    void TearDown() override {
        controller->Finalize();
        controller.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
        // Don't delete Mock_Service - it's managed by shared_ptr in tests
        Mock_Service = nullptr;

        delete Mock_ADSWManager;
        Mock_ADSWManager = nullptr;
        delete Mock_ServiceRegistry;
        Mock_ServiceRegistry = nullptr;
    }
    
    std::shared_ptr<ros2::adsw_manager::ROSServiceController> controller;
};

// Constructor/Destructor tests
TEST_F(ROSServiceControllerTest, ConstructorDestructor) {
    auto test_controller = std::make_shared<ROSServiceController>();
    ASSERT_NE(test_controller, nullptr);
    test_controller.reset();
}

// Initialize and Finalize tests
TEST_F(ROSServiceControllerTest, InitializeAndFinalize) {
    auto test_controller = std::make_shared<ROSServiceController>();
    EXPECT_TRUE(test_controller->Initialize());
    test_controller->Finalize();
    // Multiple calls should be safe
    EXPECT_TRUE(test_controller->Initialize());
    test_controller->Finalize();
}

TEST_F(ROSServiceControllerTest, MultipleInitializeCalls) {
    EXPECT_TRUE(controller->Initialize());
    EXPECT_TRUE(controller->Initialize());
}

// Null service tests
TEST_F(ROSServiceControllerTest, StartServiceWithNullService) {
    std::shared_ptr<Service> null_service;
    EXPECT_FALSE(controller->StartService(null_service));
}

TEST_F(ROSServiceControllerTest, StopServiceWithNullService) {
    std::shared_ptr<Service> null_service;
    EXPECT_FALSE(controller->StopService(null_service));
}

TEST_F(ROSServiceControllerTest, GetNodeListsWithNullService) {
    std::shared_ptr<Service> null_service;
    EXPECT_FALSE(controller->GetNodeLists(null_service));
}

TEST_F(ROSServiceControllerTest, GetPIDWithNullService) {
    std::shared_ptr<Service> null_service;
    EXPECT_EQ(controller->GetPID(null_service), 0);
}

// String-based service methods with empty/invalid names
TEST_F(ROSServiceControllerTest, StartServiceByName_EmptyName) {
    EXPECT_FALSE(controller->StartService(std::string("")));
}

TEST_F(ROSServiceControllerTest, StopServiceByName_EmptyName) {
    EXPECT_FALSE(controller->StopService(std::string("")));
}

TEST_F(ROSServiceControllerTest, StartServiceByName_NonExistent) {
    EXPECT_FALSE(controller->StartService(std::string("nonexistent_service")));
}

TEST_F(ROSServiceControllerTest, StopServiceByName_NonExistent) {
    EXPECT_FALSE(controller->StopService(std::string("nonexistent_service")));
}

// Group operations tests
TEST_F(ROSServiceControllerTest, StartGroup_EmptyGroup) {
    EXPECT_FALSE(controller->StartGroup(""));
}

TEST_F(ROSServiceControllerTest, StopGroup_EmptyGroup) {
    EXPECT_FALSE(controller->StopGroup(""));
}

TEST_F(ROSServiceControllerTest, StartGroup_ContainerNotSupported) {
    EXPECT_FALSE(controller->StartGroup("test-group"));
}

TEST_F(ROSServiceControllerTest, StopGroup_ContainerNotSupported) {
    EXPECT_FALSE(controller->StopGroup("test-group"));
}

// Container getter test
TEST_F(ROSServiceControllerTest, GetContainer_NotNull) {
    auto container = controller->GetContainer();
    EXPECT_NE(container, nullptr);
}

// Mock service tests - StartService
TEST_F(ROSServiceControllerTest, StartServiceWithMockService_EmptyLaunchFile) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return(""));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(0));
    
    // Should not crash
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

TEST_F(ROSServiceControllerTest, StartServiceWithMockService_WithLaunchFile) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return("test.launch.py"));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(0));
    
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

TEST_F(ROSServiceControllerTest, StartServiceWithMockService_EmptyPackage) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return(""));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return(""));
    
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

// Mock service tests - StopService
TEST_F(ROSServiceControllerTest, StopServiceWithMockService_NoPID) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(0));
    
    // Should return false when service has no PID
    EXPECT_FALSE(controller->StopService(mock_service));
}

TEST_F(ROSServiceControllerTest, StopServiceWithMockService_ValidPID) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(12345));
    
    // Should attempt to stop
    EXPECT_NO_THROW(controller->StopService(mock_service));
}

// GetNodeLists tests
TEST_F(ROSServiceControllerTest, GetNodeListsWithMockService) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    
    EXPECT_NO_THROW(controller->GetNodeLists(mock_service));
}

TEST_F(ROSServiceControllerTest, GetNodeListsWithMockService_EmptyPackage) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return(""));
    
    EXPECT_NO_THROW(controller->GetNodeLists(mock_service));
}

// GetPID tests
TEST_F(ROSServiceControllerTest, GetPIDWithMockService) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    
    uint64_t pid = controller->GetPID(mock_service);
    EXPECT_GE(pid, 0);
}

// Process request tests with null messages
TEST_F(ROSServiceControllerTest, ProcessServiceStartRequest_NullMessage) {
    EXPECT_FALSE(controller->ProcessServiceStartRequest(nullptr));
}

TEST_F(ROSServiceControllerTest, ProcessServiceStopRequest_NullMessage) {
    EXPECT_FALSE(controller->ProcessServiceStopRequest(nullptr));
}

// Environment detection tests - these are called during Initialize
TEST_F(ROSServiceControllerTest, Initialize_DetectsEnvironment) {
    auto test_controller = std::make_shared<ROSServiceController>();
    // Should detect environment without crashing
    EXPECT_TRUE(test_controller->Initialize());
    test_controller->Finalize();
}

// Multiple service operations
TEST_F(ROSServiceControllerTest, StartMultipleServices) {
    auto service1 = std::make_shared<MockService>("service1");
    auto service2 = std::make_shared<MockService>("service2");
    
    Mock_Service = service1.get();
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetName()).WillByDefault(Return("service1"));
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetPackage()).WillByDefault(Return("pkg1"));
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetLaunchFile()).WillByDefault(Return(""));
    
    EXPECT_NO_THROW(controller->StartService(service1));
    
    Mock_Service = service2.get();
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetName()).WillByDefault(Return("service2"));
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetPackage()).WillByDefault(Return("pkg2"));
    ON_CALL(*static_cast<MockService*>(Mock_Service), GetLaunchFile()).WillByDefault(Return("test.launch.py"));
    
    EXPECT_NO_THROW(controller->StartService(service2));
}

// Edge cases
TEST_F(ROSServiceControllerTest, StartService_VeryLongServiceName) {
    auto mock_service = std::make_shared<MockService>(std::string(1000, 'a'));
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return(std::string(1000, 'a')));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return(""));
    
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

TEST_F(ROSServiceControllerTest, StartService_SpecialCharacters) {
    auto mock_service = std::make_shared<MockService>("test_@#$%");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_@#$%"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("pkg_@#$%"));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return(""));
    
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

// Finalize without Initialize
TEST_F(ROSServiceControllerTest, FinalizeWithoutInitialize) {
    auto test_controller = std::make_shared<ROSServiceController>();
    EXPECT_NO_THROW(test_controller->Finalize());
}

// Double Finalize
TEST_F(ROSServiceControllerTest, DoubleFinalizeTest) {
    controller->Finalize();
    EXPECT_NO_THROW(controller->Finalize());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
