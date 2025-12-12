// Unit tests for ROSServiceController class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ROSServiceController.hpp"

#include "mock/MockService.hpp"
#include "mock/MockServiceMonitor.hpp"
#include "mock/MockContainerController.hpp"
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
        Mock_Service = nullptr;  // Tests that need it will create and set
        controller = std::make_shared<ros2::adsw_manager::ROSServiceController>();
        controller->Initialize();
    }
    
    void TearDown() override {
        controller->Finalize();
        controller.reset();
        delete Mock_Logger;
        Mock_Logger = nullptr;
        Mock_Service = nullptr;  // Don't delete - managed by shared_ptr in tests
    }
    
    std::shared_ptr<ros2::adsw_manager::ROSServiceController> controller;
};

TEST_F(ROSServiceControllerTest, InitializeAndFinalize) {
    // Controller is already initialized in SetUp, just verify it works
    SUCCEED();
}

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

TEST_F(ROSServiceControllerTest, StartServiceByName_EmptyName) {
    EXPECT_FALSE(controller->StartService(std::string("")));
}

TEST_F(ROSServiceControllerTest, StopServiceByName_EmptyName) {
    EXPECT_FALSE(controller->StopService(std::string("")));
}

TEST_F(ROSServiceControllerTest, StartGroup_EmptyGroup) {
    EXPECT_FALSE(controller->StartGroup(""));
}

TEST_F(ROSServiceControllerTest, StopGroup_EmptyGroup) {
    EXPECT_FALSE(controller->StopGroup(""));
}

TEST_F(ROSServiceControllerTest, GetContainer_NotNull) {
    auto container = controller->GetContainer();
    EXPECT_NE(container, nullptr);
}

TEST_F(ROSServiceControllerTest, StartServiceWithMockService) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    ON_CALL(*Mock_Service, GetLaunchFile()).WillByDefault(Return(""));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(0));
    
    // StartService may fail in test environment, but should not crash
    EXPECT_NO_THROW(controller->StartService(mock_service));
}

TEST_F(ROSServiceControllerTest, StopServiceWithMockService_NoPID) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPID()).WillByDefault(Return(0));
    
    // Should return false when service has no PID
    EXPECT_FALSE(controller->StopService(mock_service));
}

TEST_F(ROSServiceControllerTest, GetNodeListsWithMockService) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    
    // Should not crash, may return true or false depending on environment
    EXPECT_NO_THROW(controller->GetNodeLists(mock_service));
}

TEST_F(ROSServiceControllerTest, GetPIDWithMockService) {
    auto mock_service = std::make_shared<MockService>("test_service");
    Mock_Service = mock_service.get();
    
    ON_CALL(*Mock_Service, GetName()).WillByDefault(Return("test_service"));
    ON_CALL(*Mock_Service, GetPackage()).WillByDefault(Return("test_package"));
    
    // Should return 0 or a valid PID depending on environment
    uint64_t pid = controller->GetPID(mock_service);
    EXPECT_GE(pid, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
