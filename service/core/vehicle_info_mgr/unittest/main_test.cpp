#include "gtest/gtest.h"
#include <thread>
#include <chrono>
#include <csignal>

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
        // The test must be run from the build directory.
        auto configPath = getEnv("VEHICLE_CONFIG_PATH").value_or("../config");

        himPath = configPath + "/viss.him"; 
        schemaPath = configPath + "/vissv3.0-schema.json";

        //config_path.append("/vissv3.0-schema.json");
        JsonSchemaValidator::init(schemaPath);
    }

    std::string himPath;
    std::string schemaPath;
};

TEST_F(MainFunctionTest, MainRunsAndStopsGracefullyOnSignal) {
    // This test verifies that the application can start up and shut down
    // gracefully when it receives an interrupt signal (like Ctrl+C).

    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        const_cast<char*>(himPath.c_str()),
        (char*)"--schema",
        const_cast<char*>(schemaPath.c_str()),
        (char*)"--grpc-provider-addr",
        (char*)"127.0.0.1:50052"
    };
    int argc = 7;

    // Run the main function in a separate thread because it contains a blocking run() loop.
    std::thread main_thread([&]() {
        EXPECT_EQ(main_test_entry(argc, argv), 0);
    });

    // Allow some time for the server to initialize.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Send an interrupt signal to the process to trigger the signal handler.
    raise(SIGINT);

    // Wait for the main thread to complete. If the signal handling is correct,
    // the app.run() loop will terminate, and the thread will join.
    if (main_thread.joinable()) {
        main_thread.join();
    } else {
        FAIL() << "Main thread is not joinable.";
    }

    // If the thread joins successfully, it means the application shut down correctly.
    SUCCEED();
}

TEST_F(MainFunctionTest, MainExitsWithInvalidHimArgument) {
    // Simulate command line arguments for the main function
    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        (char*)"../non_existent_file.him",
        (char*)"--schema",
        const_cast<char*>(schemaPath.c_str())
    };
    int argc = 5;

    // The main function should call the VissServerApp constructor with the invalid path.
    // The VissServerApp constructor is expected to call exit(1) when the file is invalid.
    // EXPECT_EXIT checks if the statement inside it calls exit() with the specified code.
    EXPECT_EXIT(
        main_test_entry(argc, argv),
        testing::ExitedWithCode(1),
        ".*" // Match any output to stderr
    );
}

TEST_F(MainFunctionTest, MainExitsWithInvalidSchemaArgument) {
    // This test verifies that the application can start up and shut down
    // gracefully when it receives an interrupt signal (like Ctrl+C).

    char* argv[] = {
        (char*)"viss-server",
        (char*)"--him",
        const_cast<char*>(himPath.c_str()),
        (char*)"--schema",
        (char*)"../non_existent_schema.json"
    };
    int argc = sizeof(argv) / sizeof(char*);

    // Run the main function in a separate thread because it contains a blocking run() loop.
    std::thread main_thread([&]() {
        EXPECT_EQ(main_test_entry(argc, argv), 0);
    });

    // Allow some time for the server to initialize.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Send an interrupt signal to the process to trigger the signal handler.
    raise(SIGINT);

    // Wait for the main thread to complete. If the signal handling is correct,
    // the app.run() loop will terminate, and the thread will join.
    if (main_thread.joinable()) {
        main_thread.join();
    } else {
        FAIL() << "Main thread is not joinable.";
    }

    // If the thread joins successfully, it means the application shut down correctly.
    SUCCEED();
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
