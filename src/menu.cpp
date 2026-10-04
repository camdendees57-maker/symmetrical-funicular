#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "And64InlineHook.hpp"

#include <EGL/egl.h>
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AxiomMenu", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "AxiomMenu", __VA_ARGS__)

static EGLBoolean (*orig_swap)(EGLDisplay, EGLSurface) = nullptr;
static bool g_ready = false;
static bool g_open = true;
static bool g_flag = false;
static float g_scale = 1.5f;
static int g_frames = 0;

static void setup_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = g_scale;
    ImGui_ImplOpenGL3_Init("#version 300 es");
    g_ready = true;
    LOGI("imgui ready");
}

static void draw_menu(int w, int h)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DeltaTime = 1.0f / 72.0f;
    io.FontGlobalScale = g_scale;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_open)
    {
        ImGui::SetNextWindowSize(ImVec2(520.0f, 380.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Axiom", &g_open, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Quest Tools menu  |  arm64  |  GLES");
        ImGui::Separator();
        ImGui::Checkbox("Flag", &g_flag);
        ImGui::SliderFloat("Scale", &g_scale, 1.0f, 2.5f, "%.2f");
        ImGui::Text("Frame %d   %dx%d", g_frames, w, h);
        ImGui::TextWrapped("OpenGL ES only. A Vulkan title will load this .so and log, but will not draw this window.");
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    g_frames++;
}

static EGLBoolean hook_swap(EGLDisplay dpy, EGLSurface surface)
{
    EGLint w = 0;
    EGLint h = 0;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    if (!g_ready && w > 0 && h > 0)
        setup_imgui();
    if (g_ready)
        draw_menu(w, h);

    return orig_swap(dpy, surface);
}

static void* boot(void*)
{
    for (int i = 0; i < 90; ++i)
    {
        void* egl = dlopen("libEGL.so", RTLD_NOW);
        void* sym = egl ? dlsym(egl, "eglSwapBuffers") : nullptr;
        if (sym)
        {
            A64HookFunction(sym, (void*)hook_swap, (void**)&orig_swap);
            LOGI("hooked eglSwapBuffers");
            return nullptr;
        }
        sleep(1);
    }
    LOGE("eglSwapBuffers not found. Vulkan-only game, or libEGL not loaded.");
    return nullptr;
}

static void start()
{
    static bool once = false;
    if (once)
        return;
    once = true;
    pthread_t t;
    pthread_create(&t, nullptr, boot, nullptr);
    pthread_detach(t);
    LOGI("loaded");
}

__attribute__((constructor)) static void on_load()
{
    start();
}

extern "C" jint JNI_OnLoad(JavaVM*, void*)
{
    start();
    return JNI_VERSION_1_6;
}
