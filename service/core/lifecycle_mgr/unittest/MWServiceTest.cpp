#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "service/MWService.hpp"
#include "service/ArgumentConfig.hpp"

//#include "mock/mock_argumentconfig.hpp" // Assuming this is where MockArgumentConfig is defined
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
    virtual void SetUp(){
        ins = new MWService(42, "TestService", "/usr/bin/test", true, {});
    }
    virtual void TearDown(){
        delete ins;
    }

    MWService* ins;
};


TEST_F(MWServiceTest, ConstructorAndGetters) {
    std::vector<ArgumentConfig> args = { ArgumentConfig("arg1", "first arg", "string", "default1"), 
                                        ArgumentConfig("arg2", "second arg", "int", "42") };
    MWService service(42, "TestService", "/usr/bin/test", true, args);

    EXPECT_EQ(service.GetName(), "TestService");
    EXPECT_EQ(service.GetPath(), "/usr/bin/test");
    EXPECT_TRUE(service.IsActive());

    std::vector<std::string> expected_args = {"default1", "42"};
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

TEST_F(MWServiceTest, EmptyArguments) {
    MWService service(3, "svc3", "/bin/svc3", true, {});
    std::vector<std::string> args = service.GetArgs();
    EXPECT_TRUE(args.empty());
}

TEST_F(MWServiceTest, MultipleArguments) {
    std::vector<ArgumentConfig> args = { 
        ArgumentConfig("arg1", "first arg", "string", "value1"), 
        ArgumentConfig("arg2", "second arg", "int", "42"),
        ArgumentConfig("arg3", "third arg", "bool", "true"),
        ArgumentConfig("arg4", "fourth arg", "float", "3.14")
    };
    MWService service(42, "TestService", "/usr/bin/test", true, args);

    std::vector<std::string> retrieved_args = service.GetArgs();
std::cout<<"Retrieved Args Size: " << retrieved_args.size() << std::endl;    
    EXPECT_EQ(retrieved_args.size(), 4);
    EXPECT_EQ(retrieved_args[0], "value1");
    EXPECT_EQ(retrieved_args[1], "42");
    EXPECT_EQ(retrieved_args[2], "true");
    EXPECT_EQ(retrieved_args[3], "3.14");
}

TEST_F(MWServiceTest, NegativePID) {
    MWService service(4, "svc4", "/bin/svc4", true, {});
    EXPECT_EQ(service.GetPID(), -1);
    
    service.SetPID(-100);
    EXPECT_EQ(service.GetPID(), -100);
}

TEST_F(MWServiceTest, ZeroPID) {
    MWService service(5, "svc5", "/bin/svc5", true, {});
    service.SetPID(1000);
    EXPECT_EQ(service.GetPID(), 1000);
    
    service.SetPID(0);
    EXPECT_EQ(service.GetPID(), 0);
}

TEST_F(MWServiceTest, LargePID) {
    MWService service(6, "svc6", "/bin/svc6", true, {});
    pid_t large_pid = 99999;
    service.SetPID(large_pid);
    EXPECT_EQ(service.GetPID(), large_pid);
}

TEST_F(MWServiceTest, EmptyServiceName) {
    MWService service(7, "", "/bin/test", true, {});
    EXPECT_EQ(service.GetName(), "");
}

TEST_F(MWServiceTest, EmptyPath) {
    MWService service(8, "empty_path_svc", "", true, {});
    EXPECT_EQ(service.GetPath(), "");
}

TEST_F(MWServiceTest, SpecialCharactersInName) {
    MWService service(9, "service-name_123.test", "/bin/test", true, {});
    EXPECT_EQ(service.GetName(), "service-name_123.test");
}

TEST_F(MWServiceTest, LongPath) {
    std::string long_path = "/very/long/path/to/executable/that/is/nested/deeply/in/filesystem/structure/bin/app";
    MWService service(10, "long_path_svc", long_path, true, {});
    EXPECT_EQ(service.GetPath(), long_path);
}

TEST_F(MWServiceTest, DifferentCodeValues) {
    MWService service1(0, "svc_zero", "/bin/test", true, {});
    MWService service2(-1, "svc_negative", "/bin/test", true, {});
    MWService service3(999999, "svc_large", "/bin/test", true, {});
    
    // Just verify construction works with different code values
    EXPECT_EQ(service1.GetName(), "svc_zero");
    EXPECT_EQ(service2.GetName(), "svc_negative");
    EXPECT_EQ(service3.GetName(), "svc_large");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

