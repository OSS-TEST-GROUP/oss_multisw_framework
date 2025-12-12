#include "gtest/gtest.h"
#include "VissServerApp.h"
#include "JsonSchemaValidator.h"
#include <thread>
#include <chrono>

// A simple test fixture for VissServerApp tests
class VissServerAppTest : public ::testing::Test {
protected:
    void SetUp() override {
        // It's crucial to have a valid HIM and schema for the app to initialize.
        // We assume these are in the standard config path relative to where the test is run.
        JsonSchemaValidator::init("../config/vissv3.0-schema.json");
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(VissServerAppTest, AppInitializesAndShutsDown) {
    // 1. Create the application instance
    // This will test if the constructor successfully initializes all components
    // without crashing.
    VissServerApp app("../config/viss.him", "127.0.0.1:50052");

    // 2. Run the app in a separate thread
    std::thread app_thread(&VissServerApp::run, &app);

    // 3. Let the app run for a short period
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 4. Call stop() to initiate shutdown
    app.stop();

    // 5. Wait for the app thread to finish
    if (app_thread.joinable()) {
        app_thread.join();
    }

    // If we reached here without crashing, the basic run/stop cycle is working.
    // The destructor of VissServerApp will also be called, which is another
    // part of the test.
    SUCCEED();
}

TEST_F(VissServerAppTest, StopIsIdempotent) {
    // 1. Create the application instance
    VissServerApp app("../config/viss.him", "127.0.0.1:50052");

    // 2. Run the app in a separate thread
    std::thread app_thread(&VissServerApp::run, &app);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 3. Call stop multiple times
    app.stop();
    app.stop(); // Calling stop again should not cause issues

    // 4. Wait for the app thread to finish
    if (app_thread.joinable()) {
        app_thread.join();
    }

    // 5. Call stop again after the thread has joined
    app.stop();

    // If no crashes or deadlocks occurred, the test passes.
    SUCCEED();
}

TEST_F(VissServerAppTest, RunIsIdempotent) {
    // 1. Create the application instance
    VissServerApp app("../config/viss.him", "127.0.0.1:50052");

    // 2. Run the app in a separate thread
    std::thread app_thread(&VissServerApp::run, &app);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 3. Calling run() again while it's running should be a no-op
    app.run();

    // 4. Stop the app
    app.stop();
    if (app_thread.joinable()) {
        app_thread.join();
    }
    
    // If no crashes or unexpected behavior occurred, the test passes.
    SUCCEED();
}

TEST_F(VissServerAppTest, InvalidFile) {
    EXPECT_EXIT({
        VissServerApp app("../InvalidFile.him", "127.0.0.1:50052");
    }, testing::ExitedWithCode(1), ".*");
}
