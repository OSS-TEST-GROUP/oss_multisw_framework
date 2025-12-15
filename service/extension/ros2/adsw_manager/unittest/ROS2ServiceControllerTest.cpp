// Unit tests for DDSTransmitter class using Google Test and Google Mock
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "control/ROSServiceController.hpp"

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

class ROSServiceControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_shared<ros2::adsw_manager::ROSServiceController>();
        controller->Initialize();
    }
    std::shared_ptr<ros2::adsw_manager::ROSServiceController> controller;
};

TEST_F(ROSServiceControllerTest, InitializeAndFinalize) {
    ASSERT_TRUE(controller->Initialize());
    controller->Finalize();
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



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
