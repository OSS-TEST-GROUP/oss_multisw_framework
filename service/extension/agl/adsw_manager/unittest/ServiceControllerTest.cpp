#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ServiceController.hpp"
#include "mock/MockAppController.hpp"
#include "mock/MockContainerController.hpp"

using namespace ::testing;
using namespace adsw_manager::agl;

class ServiceControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<NiceMock<MockAppController>> app;
    std::shared_ptr<NiceMock<MockContainerController>> cont;
    std::unique_ptr<ServiceController> sut;

    void SetUp() override {
        app  = std::make_shared<NiceMock<MockAppController>>();
        cont = std::make_shared<NiceMock<MockContainerController>>();
        sut  = std::make_unique<ServiceController>(app.get(), cont.get());
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ServiceControllerTest, Initialize_Succeeds_WhenBothDepsInitSucceed) {
    EXPECT_CALL(*app, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*cont, Initialize()).WillOnce(Return(true));

    EXPECT_TRUE(sut->Initialize());
}

TEST_F(ServiceControllerTest, Initialize_Fails_WhenAppControllerInitFails) {
    EXPECT_CALL(*app, Initialize()).WillOnce(Return(false));
    EXPECT_CALL(*cont, Initialize()).Times(0);  // 호출되지 않아야 함

    EXPECT_FALSE(sut->Initialize());
}

TEST_F(ServiceControllerTest, Initialize_Fails_WhenContainerControllerInitFails) {
    InSequence s;
    EXPECT_CALL(*app, Initialize()).WillOnce(Return(true));
    EXPECT_CALL(*cont, Initialize()).WillOnce(Return(false));

    EXPECT_FALSE(sut->Initialize());
}

// ----------------------------- StartService ---------------------------

TEST_F(ServiceControllerTest, StartService_ContainerRunning_AppNotRunning_LaunchSuccess_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1", "cont1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerRunning_AppNotRunning_LaunchFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1", "cont1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerRunning_AppAlreadyRunning_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerStopped_StartContainerFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StartContainer("cont1")).WillOnce(Return(false));

    // 이후 경로는 타지 않음
    EXPECT_CALL(*app, IsAppRunning(_, _)).Times(0);
    EXPECT_CALL(*app, LaunchApp(_, _)).Times(0);

    EXPECT_FALSE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerStopped_StartsContainer_AppNotRunning_LaunchSuccess_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StartContainer("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1", "cont1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerStopped_StartsContainer_AppNotRunning_LaunchFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StartContainer("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1", "cont1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StartService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StartService_ContainerStopped_StartsContainer_AppAlreadyRunning_FallsThroughAndReturnsFalse_CurrentBehavior) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StartContainer("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(true));

    EXPECT_FALSE(sut->StartService("app1", "cont1"));
}

// ----------------------------- StopService ----------------------------

TEST_F(ServiceControllerTest, StopService_ContainerNotRunning_ReturnsTrue) {
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(false));
    EXPECT_TRUE(sut->StopService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StopService_ContainerRunning_AppRunning_TerminateOk_StopOk_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  TerminateApp("app1", "cont1")).WillOnce(Return(true));
    EXPECT_CALL(*cont, StopContainer("cont1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StopService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StopService_ContainerRunning_AppRunning_TerminateOk_StopFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  TerminateApp("app1", "cont1")).WillOnce(Return(true));
    EXPECT_CALL(*cont, StopContainer("cont1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StopService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StopService_ContainerRunning_AppRunning_TerminateFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  TerminateApp("app1", "cont1")).WillOnce(Return(false));
    // StopContainer는 호출되지 않음
    EXPECT_CALL(*cont, StopContainer(_)).Times(0);

    EXPECT_FALSE(sut->StopService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StopService_ContainerRunning_AppNotRunning_StopOk_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StopContainer("cont1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StopService("app1", "cont1"));
}

TEST_F(ServiceControllerTest, StopService_ContainerRunning_AppNotRunning_StopFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*cont, IsContainerRunning("cont1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  IsAppRunning("app1", "cont1")).WillOnce(Return(false));
    EXPECT_CALL(*cont, StopContainer("cont1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StopService("app1", "cont1"));
}
