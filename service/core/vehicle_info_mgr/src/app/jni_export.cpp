#include <jni.h>
#include <string>
#include <memory>
#include <thread>
#include <stdexcept>
#include <android/log.h>
#include "VissServerApp.h"
#include "JsonSchemaValidator.h"

#define LOG_TAG "VISS_JNI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Global pointer to the VissServerApp instance
static std::unique_ptr<VissServerApp> g_viss_app;
static std::unique_ptr<std::thread> g_viss_thread;

// Helper to convert jstring to std::string
std::string jstring_to_string(JNIEnv* env, jstring jstr) {
    if (!jstr) {
        return "";
    }
    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
    std::string str = cstr;
    env->ReleaseStringUTFChars(jstr, cstr);
    return str;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zeus_myapplication_VissService_startVissServer(
        JNIEnv* env,
        jobject /* this */,
        jstring himPath,
        jstring schemaPath,
        jstring grpcProviderAddr) {

    if (g_viss_app) {
        LOGI("VISS Server is already running.");
        return 0; // Already running, report success
    }

    std::string him_path_str = jstring_to_string(env, himPath);
    std::string schema_path_str = jstring_to_string(env, schemaPath);
    std::string grpc_provider_addr_str = jstring_to_string(env, grpcProviderAddr);

    if (him_path_str.empty() || schema_path_str.empty() || grpc_provider_addr_str.empty()) {
        LOGE("Path to config files or gRPC provider address is empty.");
        return -1;
    }

    g_viss_thread = std::make_unique<std::thread>([him_path_str, schema_path_str, grpc_provider_addr_str]() {
        try {
            LOGI("VISS Server thread started with HIM: %s, Addr: %s", him_path_str.c_str(), grpc_provider_addr_str.c_str());
            JsonSchemaValidator::init(schema_path_str);
            g_viss_app = std::make_unique<VissServerApp>(him_path_str, grpc_provider_addr_str);
            g_viss_app->run();
            LOGI("VISS Server thread finished.");
        } catch (const std::exception& e) {
            LOGE("!!! Exception in VISS Server thread: %s", e.what());
            g_viss_app.reset(); // Clean up on failure
        } catch (...) {
            LOGE("!!! Unknown exception in VISS Server thread.");
            g_viss_app.reset(); // Clean up on failure
        }
    });

    return 0; // Success
}

extern "C" JNIEXPORT jint JNICALL
Java_com_zeus_myapplication_VissService_stopVissServer(
        JNIEnv* env,
        jobject /* this */) {
    
    LOGI("Stopping VISS Server...");
    try {
        // 1. Signal the VissServerApp to shut down. This call is non-blocking
        //    and will push a signal to the shutdown queue, unblocking VissServerApp::run().
        if (g_viss_app) {
            g_viss_app->stop();
        }

        // 2. Wait for the main server thread (g_viss_thread), which is executing VissServerApp::run(),
        //    to finish its execution. This join will now succeed because stop() unblocked run().
        if (g_viss_thread && g_viss_thread->joinable()) {
            g_viss_thread->join();
        }

        // 3. Now that the main server thread has finished, we can safely destroy the VissServerApp object.
        //    The VissServerApp destructor will then proceed to stop and join all component threads.
        g_viss_app.reset();
        g_viss_thread.reset();

        LOGI("VISS Server stopped successfully.");
        return 0; // Success
    } catch (const std::exception& e) {
        LOGE("!!! Exception while stopping VISS Server: %s", e.what());
        return -1; // Failure
    } catch (...) {
        LOGE("!!! Unknown exception while stopping VISS Server.");
        return -1; // Failure
    }
}
