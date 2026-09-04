#include <jni.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)
void InstallPresentHook();
void HoldableTick();
static void Boot() {
    LOGI("holdable boot unity=%s", UNITY_VERSION_STR);
    InstallPresentHook();
}
extern "C" jint JNI_OnLoad(JavaVM*, void*) { Boot(); return JNI_VERSION_1_6; }
__attribute__((constructor)) static void ctor() { Boot(); }
