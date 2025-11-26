#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "TestPrinter.hpp"

#include "ServiceController.hpp"
#include "mock/MockAppController.hpp"

using namespace ::testing;
using namespace adsw_manager::agl;

class ServiceControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<NiceMock<MockAppController>> app;
    std::unique_ptr<ServiceController> sut;

    void SetUp() override {
        app  = std::make_shared<NiceMock<MockAppController>>();
        sut  = std::make_unique<ServiceController>(app.get());
    }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ServiceControllerTest, Initialize_Succeeds_WhenBothDepsInitSucceed) {
    EXPECT_CALL(*app, Initialize()).WillOnce(Return(true));

    EXPECT_TRUE(sut->Initialize());
}

TEST_F(ServiceControllerTest, Initialize_Fails_WhenAppControllerInitFails) {
    EXPECT_CALL(*app, Initialize()).WillOnce(Return(false));

    EXPECT_FALSE(sut->Initialize());
}

// ----------------------------- StartService ---------------------------

TEST_F(ServiceControllerTest, StartService_AppNotRunning_LaunchSuccess_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StartService("app1"));
}

TEST_F(ServiceControllerTest, StartService_AppNotRunning_LaunchFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(false));
    EXPECT_CALL(*app,  LaunchApp("app1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StartService("app1"));
}

TEST_F(ServiceControllerTest, StartService_AppAlreadyRunning_ReturnsTrue) {
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StartService("app1"));
}

// ----------------------------- StopService ----------------------------

TEST_F(ServiceControllerTest, StopService_AppRunning_TerminateOk_StopOk_ReturnsTrue) {
    InSequence s;
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  TerminateApp("app1")).WillOnce(Return(true));

    EXPECT_TRUE(sut->StopService("app1"));
}

TEST_F(ServiceControllerTest, StopService_AppRunning_TerminateFail_ReturnsFalse) {
    InSequence s;
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(true));
    EXPECT_CALL(*app,  TerminateApp("app1")).WillOnce(Return(false));

    EXPECT_FALSE(sut->StopService("app1"));
}

TEST_F(ServiceControllerTest, StopService_AppNotRunning_StopOk_ReturnsTrue) {
    EXPECT_CALL(*app,  IsAppRunning("app1")).WillOnce(Return(false));

    EXPECT_TRUE(sut->StopService("app1"));
}
