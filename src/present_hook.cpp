#include "holdable_menu.h"
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)

static VkResult (*orig_present)(VkQueue, const VkPresentInfoKHR*) = nullptr;
static VkResult hooked_present(VkQueue q, const VkPresentInfoKHR* info) {
    HoldableOnPresent();
    return orig_present ? orig_present(q, info) : VK_SUCCESS;
}

static void* hunt(void*) {
    for (int i=0;i<100 && !orig_present;++i) {
        void* vk = dlopen("libvulkan.so", RTLD_NOW);
        if (vk) {
            auto inst = (PFN_vkGetInstanceProcAddr)dlsym(vk, "vkGetInstanceProcAddr");
            (void)inst;
            auto real = (VkResult(*)(VkQueue,const VkPresentInfoKHR*))dlsym(vk, "vkQueuePresentKHR");
            if (real && real != hooked_present) {
                orig_present = real;
                LOGI("vkQueuePresentKHR @ %p unity=%s", (void*)real, UNITY_VERSION_STR);
                // GOT/PLT patch is the injector job; we export the hook symbol.
                break;
            }
        }
        usleep(150*1000);
    }
    return nullptr;
}

extern "C" VkResult vkQueuePresentKHR(VkQueue q, const VkPresentInfoKHR* info) {
    HoldableOnPresent();
    if (orig_present) return orig_present(q, info);
    static auto real = (VkResult(*)(VkQueue,const VkPresentInfoKHR*))dlsym(RTLD_NEXT, "vkQueuePresentKHR");
    return real ? real(q, info) : VK_SUCCESS;
}

void InstallPresentHook() {
    pthread_t t; pthread_create(&t,nullptr,hunt,nullptr); pthread_detach(t);
}
