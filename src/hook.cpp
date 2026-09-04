#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusImGui", ##__VA_ARGS__)

static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay, EGLSurface) = nullptr;
static bool g_inited = false;
static int g_w = 0, g_h = 0;

static void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 300 es");
    g_inited = true;
    LOGI("imgui gl es3 init ok unity=%s", UNITY_VERSION_STR);
}

static void DrawMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(40, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 280), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("TagtusVR Overlay")) {
        ImGui::Text("Unity %s  arm64", UNITY_VERSION_STR);
        ImGui::Text("fb %d x %d", g_w, g_h);
        ImGui::Separator();
        static bool on = true;
        ImGui::Checkbox("menu live", &on);
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w > 0 && h > 0) {
        g_w = w; g_h = h;
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)w, (float)h);
    }
    if (!g_inited) InitImGui();
    if (g_inited) DrawMenu();
    return orig_eglSwapBuffers(dpy, surf);
}

static void* hook_thread(void*) {
    for (int i = 0; i < 80 && !orig_eglSwapBuffers; ++i) {
        void* egl = dlopen("libEGL.so", RTLD_NOW);
        if (egl) {
            auto real = (EGLBoolean(*)(EGLDisplay, EGLSurface))dlsym(egl, "eglSwapBuffers");
            if (real && real != hooked_eglSwapBuffers) {
                orig_eglSwapBuffers = real;
                // soft hook via LD_PRELOAD / injector symbol interpose preferred;
                // injector should patch GOT/PLT of libunity/libEGL to hooked_eglSwapBuffers
                LOGI("eglSwapBuffers @ %p", (void*)real);
                break;
            }
        }
        usleep(200 * 1000);
    }
    return nullptr;
}

void InstallGfxHooks() {
    pthread_t t;
    pthread_create(&t, nullptr, hook_thread, nullptr);
    pthread_detach(t);
}
