#include <jni.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)
void InstallPresentHook();
void ResolveXrHooks();
void Il2CppBind();
static void Boot() {
    LOGI("holdable+xr+il2cpp boot unity=%s", UNITY_VERSION_STR);
    ResolveXrHooks();
    Il2CppBind();
    InstallPresentHook();
}
extern "C" jint JNI_OnLoad(JavaVM*, void*) { Boot(); return JNI_VERSION_1_6; }
__attribute__((constructor)) static void ctor() { Boot(); }
