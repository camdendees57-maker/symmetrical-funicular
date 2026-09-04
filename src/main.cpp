#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusImGui", ##__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "TagtusImGui", ##__VA_ARGS__)

extern void InstallGfxHooks();

static void Bootstrap() {
    LOGI("imgui arm64 boot Unity=%s", UNITY_VERSION_STR);
    InstallGfxHooks();
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void*) {
    (void)vm;
    Bootstrap();
    return JNI_VERSION_1_6;
}

__attribute__((constructor))
static void on_load() {
    Bootstrap();
}
