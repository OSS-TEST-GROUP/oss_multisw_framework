#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "service/MWService.hpp"
#include "mock/mock_argumentconfig.hpp" // Assuming this is where MockArgumentConfig is defined
#include "unittest/mock/mock_mw_service_manager.hpp" // Assuming this is where MockMWServiceManager is defined


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


class MWServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    void TearDown() override {
        // Cleanup code if needed
    }
};


TEST_F(MWServiceTest, ConstructorAndGetters) {
    std::vector<ArgumentConfig> args = { ArgumentConfig("arg1"), ArgumentConfig("arg2") };
    MWService service(42, "TestService", "/usr/bin/test", true, args);

    EXPECT_EQ(service.GetName(), "TestService");
    EXPECT_EQ(service.GetPath(), "/usr/bin/test");
    EXPECT_TRUE(service.IsActive());

    std::vector<std::string> expected_args = {"arg1", "arg2"};
    EXPECT_EQ(service.GetArgs(), expected_args);
}

TEST_F(MWServiceTest, SetAndGetPID) {
    MWService service(1, "svc", "/bin/svc", false, {});
    EXPECT_EQ(service.GetPID(), -1);

    service.SetPID(1234);
    EXPECT_EQ(service.GetPID(), 1234);
}

TEST_F(MWServiceTest, InactiveService) {
    MWService service(2, "svc2", "/bin/svc2", false, {});
    EXPECT_FALSE(service.IsActive());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

