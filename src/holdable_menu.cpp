#include "holdable_menu.h"
#include "xr_pose.h"
#include "imgui_panel.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)

static bool g_prevTrig=false;

void HoldableOnPresent() {
    Hands h = ReadHands();
    bool poke = h.rTrig && !g_prevTrig;
    g_prevTrig = h.rTrig;
    ImGuiPanel_Frame(h.head, h.left, h.right, poke || h.rTrig);
}
