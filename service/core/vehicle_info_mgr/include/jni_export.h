#ifndef JNI_EXPORT_H
#define JNI_EXPORT_H

#ifdef __ANDROID__

#include <jni.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_example_viss_VissService
 * Method:    startVissServer
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT jint JNICALL Java_com_obigo_viss_VissServer_startVissServer(
    JNIEnv* env,
    jobject /* this */,
    jstring himPath,
    jstring schemaPath);

/*
 * Class:     com_example_viss_VissService
 * Method:    stopVissServer
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_obigo_viss_VissServer_stopVissServer(
    JNIEnv* env,
    jobject /* this */);

#ifdef __cplusplus
}
#endif

#endif // __ANDROID__

#endif // JNI_EXPORT_H
