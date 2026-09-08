#include "imgui_panel.h"
#include "imgui_vr.h"
#include "spawn.h"
#include "il2cpp_api.h"
#include "Il2CppMethodNames.hpp"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusUI", ##__VA_ARGS__)

#ifdef HAS_IMGUI
#include "imgui.h"
#endif

static bool g_init=false;
static bool g_prevPoke=false;

void ImGuiPanel_Init(){
    if (g_init) return;
    ImGuiVR_Init(0.70f, 0.50f, 1.15f);
#ifdef HAS_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(420, 320);
    io.DeltaTime = 1.f/72.f;
#endif
    g_init = true;
}

void ImGuiPanel_Frame(const Pose& head, const Pose& left, const Pose& right, bool poke){
    ImGuiPanel_Init();
    ImGuiVR_SetHead(head);
    ImGuiVR_SetLeftHand(left);

    float world[16];
    ImGuiVR_BuildWorld(world);
    Pose board{};
    board.valid = true;
    board.p = {world[12], world[13], world[14]};
    board.q = (ImGuiVR_GetFollowMode()==IMGUIVR_FOLLOW_LEFT_HAND && left.valid) ? left.q : (head.valid?head.q:Quat{0,0,0,1});

#ifdef HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = poke;
    if (board.valid && right.valid) {
        Vec3 origin = right.p;
        Vec3 dir = qrot(right.q, {0,0,-1});
        Vec3 n = qrot(board.q, {0,0,1});
        float denom = dot(n, dir);
        if (fabsf(denom) > 1e-4f) {
            float t = dot(n, sub(board.p, origin)) / denom;
            if (t>0 && t<0.45f) {
                Vec3 hit = add(origin, mul(dir, t));
                Vec3 local = qrot(Quat{-board.q.x,-board.q.y,-board.q.z,board.q.w}, sub(hit, board.p));
                float u = (local.x + 0.28f) / 0.56f;
                float v = (0.20f - local.y) / 0.40f;
                io.MousePos = ImVec2(u * io.DisplaySize.x, v * io.DisplaySize.y);
            }
        }
    }
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("TAGTUSVR", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);

    if (ImGuiVR_GetFollowMode()==IMGUIVR_FOLLOW_LOOK) {
        if (ImGui::Button("SNAP TO LEFT HAND", ImVec2(-1, 28))) ImGuiVR_SetFollowMode(IMGUIVR_FOLLOW_LEFT_HAND);
    } else {
        if (ImGui::Button("FOLLOW LOOK", ImVec2(-1, 28))) ImGuiVR_SetFollowMode(IMGUIVR_FOLLOW_LOOK);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("SYMBOLS");
    ImGui::Text("ready=%d", (int)Il2CppReady());
    ImGui::Text("domain_get  %s", symbol_il2cpp_domain_get);
    ImGui::Text("invoke      %s", symbol_il2cpp_runtime_invoke);
    ImGui::Text("from_name   %s", symbol_il2cpp_class_from_name);
    ImGui::Text("method_get  %s", symbol_il2cpp_class_get_method_from_name);
    ImGui::Text("string_new  %s", symbol_il2cpp_string_new);
    ImGui::Text("attach      %s", symbol_il2cpp_thread_attach);
    ImGui::Text("for_each    %s", symbol_il2cpp_class_for_each);

    ImGui::Separator();
    ImGui::Text("SPAWN  now=%s  hits=%d", SpawnCurrentItem(), SpawnHitCount());
    if (ImGui::Button("SCAN METHODS", ImVec2(-1, 24))) SpawnForceScan();
    for (int i=0;i<SpawnItemCount();++i) {
        char lab[64];
        snprintf(lab, 64, "%s%s", i==0&&false?"":"", SpawnItemName(i));
        if (ImGui::Selectable(SpawnItemName(i), i /*cmp*/ == 0 && false ? false : (SpawnCurrentItem()==SpawnItemName(i) || strcmp(SpawnCurrentItem(), SpawnItemName(i))==0)))
            SpawnSelect(i);
    }
    if (ImGui::Button("SPAWN ITEM", ImVec2(-1, 32))) {
        bool ok = SpawnDo();
        LOGI("imgui spawn %s ok=%d", SpawnCurrentItem(), (int)ok);
    }
    ImGui::BeginChild("hits", ImVec2(0, 80), true);
    for (int i=0;i<SpawnHitCount();++i)
        ImGui::Text("%s.%s argc=%d", SpawnHitClass(i), SpawnHitMethod(i), SpawnHitArgc(i));
    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
#else
    if (poke && !g_prevPoke) {
        if (!ImGuiVR_TryToggleFromPoke(board, right)) {
            /* fallback poke strip: lower half = spawn */
            Vec3 origin = right.p;
            Vec3 dir = qrot(right.q, {0,0,-1});
            Vec3 n = qrot(board.q, {0,0,1});
            float denom = dot(n, dir);
            if (fabsf(denom) > 1e-4f) {
                float t = dot(n, sub(board.p, origin)) / denom;
                if (t>0 && t<0.35f) {
                    Vec3 hit = add(origin, mul(dir, t));
                    Vec3 local = qrot(Quat{-board.q.x,-board.q.y,-board.q.z,board.q.w}, sub(hit, board.p));
                    if (local.y < 0.02f) SpawnDo();
                    else SpawnCycleItem();
                }
            }
        }
    }
#endif
    g_prevPoke = poke;
}
