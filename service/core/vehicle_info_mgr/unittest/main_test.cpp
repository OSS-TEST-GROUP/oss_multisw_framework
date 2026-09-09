#include "gtest/gtest.h"
#include <thread>
#include <chrono>
#include <csignal>
#include <libwebsockets.h>

class GlobalSSLTestEnvironment : public ::testing::Environment {
public:
    struct lws_context* dummy_ssl_ctx = nullptr;

    void SetUp() override {
        struct lws_context_creation_info info;
        memset(&info, 0, sizeof(info));
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.port = CONTEXT_PORT_NO_LISTEN;
        dummy_ssl_ctx = lws_create_context(&info);
    }

    void TearDown() override {
        if (dummy_ssl_ctx) {
            lws_context_destroy(dummy_ssl_ctx);
            dummy_ssl_ctx = nullptr;
        }
    }
};

static ::testing::Environment* const ssl_env =
    ::testing::AddGlobalTestEnvironment(new GlobalSSLTestEnvironment);

// By defining 'main' before including main.cpp, we can rename its main function
// and call it from our test cases. This allows us to test the argument parsing
// and initialization flow of the application's entry point.
#define main main_test_entry
#include "../src/app/main.cpp"
#undef main

// Test fixture for main function tests
class MainFunctionTest : public ::testing::Test {
protected:
    std::optional<std::string> getEnv(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        return val ? std::optional<std::string>(val) : std::nullopt;
    }

    void SetUp() override {
        ::testing::FLAGS_gtest_death_test_style = "threadsafe";
        auto configPath = getEnv("VEHICLE_CONFIG_PATH").value_or("../config");

        himPath = configPath + "/viss.him"; 
        schemaPath = configPath + "/vissv3.0-schema.json";

        JsonSchemaValidator::init(schemaPath);
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::string himPath;
    std::string schemaPath;
};

TEST_F(MainFunctionTest, MainRunsAndStopsGracefullyOnSignal) {
    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        const_cast<char*>(himPath.c_str()),
        (char*)"--schema",
        const_cast<char*>(schemaPath.c_str())
    };
    int argc = 5;

    std::thread main_thread([&]() {
        EXPECT_EQ(main_test_entry(argc, argv), 0);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    signal_handler(SIGINT);

    if (main_thread.joinable()) {
        main_thread.join();
    } else {
        FAIL() << "Main thread is not joinable.";
    }

    SUCCEED();
}

TEST_F(MainFunctionTest, MainExitsWithInvalidHimArgument) {
    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        (char*)"../non_existent_file.him",
        (char*)"--schema",
        const_cast<char*>(schemaPath.c_str())
    };
    int argc = 5;

    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*"
    );
}

TEST_F(MainFunctionTest, MainExitsWithInvalidSchemaArgument) {
    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        const_cast<char*>(himPath.c_str()),
        (char*)"--schema"
    };
    int argc = 4;

    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*"
    );
}





TEST_F(MainFunctionTest, MainCallWithoutHimfile) {
    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        //const_cast<char*>(himPath.c_str()),
        //(char*)"--schema",
        //const_cast<char*>(schemaPath.c_str()),
        //(char*)"--grpc-provider-addr",
        //(char*)"127.0.0.1:50052"
    };
    int argc = sizeof(argv) / sizeof(char*);

    // The main function should call the VissServerApp constructor with the invalid path.
    // The VissServerApp constructor is expected to call exit(1) when the file is invalid.
    // EXPECT_EXIT checks if the statement inside it calls exit() with the specified code.
    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*" // Match any output to stderr
    );
}

TEST_F(MainFunctionTest, MainCallWithoutSchemafile) {
    char* argv[] = {
        (char*)"viss-server",
        //(char*)"--him",
        //(char*)"../non_existent_file.him",
        (char*)"--schema",
        //const_cast<char*>(schemaPath.c_str()),
        //(char*)"--grpc-provider-addr",
        //(char*)"127.0.0.1:50052"
    };
    int argc = sizeof(argv) / sizeof(char*);

    // The main function should call the VissServerApp constructor with the invalid path.
    // The VissServerApp constructor is expected to call exit(1) when the file is invalid.
    // EXPECT_EXIT checks if the statement inside it calls exit() with the specified code.
    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*" // Match any output to stderr
    );
}

TEST_F(MainFunctionTest, MainCallWithoutProviderAddr) {
    char* argv[] = {
        (char*)"viss-server",
        //(char*)"--him",
        //(char*)"../non_existent_file.him",
        //(char*)"--schema",
        //const_cast<char*>(schemaPath.c_str()),
        (char*)"--grpc-provider-addr",
        //(char*)"127.0.0.1:50052"
    };
    int argc = sizeof(argv) / sizeof(char*);

    // The main function should call the VissServerApp constructor with the invalid path.
    // The VissServerApp constructor is expected to call exit(1) when the file is invalid.
    // EXPECT_EXIT checks if the statement inside it calls exit() with the specified code.
    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*" // Match any output to stderr
    );
}
