#include <jni.h>
#include <string>
#include <memory>
#include <thread>
#include <stdexcept>
//#include <android/log.h>
#include "VissServerApp.h"
#include "JsonSchemaValidator.h"

//#define LOG_TAG "VISS_JNI"
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

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
Java_com_obigo_viss_VissServer_startVissServer(
        JNIEnv* env,
        jobject /* this */,
        jstring himPath,
        jstring schemaPath) {

    if (g_viss_app) {
        LOG_INFO << "VISS Server is already running.";
        return 0; // Already running, report success
    }

    std::string him_path_str = jstring_to_string(env, himPath);
    std::string schema_path_str = jstring_to_string(env, schemaPath);

    if (him_path_str.empty() || schema_path_str.empty()) {
        LOG_INFO << "Path to config files.";
        return -1;
    }

    g_viss_thread = std::make_unique<std::thread>([him_path_str, schema_path_str]() {
        try {
            LOG_INFO << "VISS Server thread started with HIM:" <<  him_path_str;
            JsonSchemaValidator::init(schema_path_str);
            g_viss_app = std::make_unique<VissServerApp>(him_path_str);
            g_viss_app->run();
            LOG_INFO << "VISS Server thread finished.";
        } catch (const std::exception& e) {
            LOG_ERROR << "!!! Exception in VISS Server thread: " << e.what();
            g_viss_app.reset(); // Clean up on failure
        } catch (...) {
            LOG_ERROR << "!!! Unknown exception in VISS Server thread.";
            g_viss_app.reset(); // Clean up on failure
        }
    });

    return 0; // Success
}

extern "C" JNIEXPORT jint JNICALL
Java_com_obigo_viss_VissServer_stopVissServer(
        JNIEnv* env,
        jobject /* this */) {

    LOG_INFO << "Stopping VISS Server...";
    try {
        if (g_viss_app) {
            g_viss_app->stop();
        }

        if (g_viss_thread && g_viss_thread->joinable()) {
            g_viss_thread->join();
        }

        g_viss_app.reset();
        g_viss_thread.reset();

        LOG_INFO << "VISS Server stopped successfully.";
        return 0; // Success
    } catch (const std::exception& e) {
        LOG_ERROR << "!!! Exception while stopping VISS Server: " << e.what();
        return -1; // Failure
    } catch (...) {
        LOG_ERROR << "!!! Unknown exception while stopping VISS Server.";
        return -1; // Failure
    }
}
