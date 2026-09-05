#include "holdable_menu.h"
#include "xr_pose.h"
#include <android/log.h>
#include <cmath>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)

bool SpawnDo();
void SpawnCycleItem();
const char* SpawnCurrentItem();

static bool g_fly=false, g_speed=false, g_noclip=false, g_esp=false, g_board=false;
static int g_page=0;
static bool g_prevTrig=false;
static float g_stickLatch=0;
static const char* kPages[] = { "HOME", "MOVE", "SPAWN", "VISUAL" };

void HoldableOnPresent() {
    Hands h = ReadHands();
    bool hold = h.lGrip || g_board;
    if (!hold) return;

    if (h.rStickX > 0.7f && g_stickLatch <= 0) { g_page = (g_page+1)&3; g_stickLatch=1; }
    if (h.rStickX < -0.7f && g_stickLatch <= 0) { g_page = (g_page+3)&3; g_stickLatch=1; }
    if (fabsf(h.rStickX) < 0.3f) g_stickLatch = 0;

    Pose board = h.left;
    if (board.valid) {
        Vec3 off = qrot(board.q, Vec3{0.08f, 0.02f, -0.12f});
        board.p = add(board.p, off);
    }

    bool poke = h.rTrig && !g_prevTrig;
    g_prevTrig = h.rTrig;
    if (poke && board.valid && h.right.valid) {
        Vec3 origin = h.right.p;
        Vec3 dir = qrot(h.right.q, Vec3{0,0,-1});
        Vec3 n = qrot(board.q, Vec3{0,0,1});
        float denom = dot(n, dir);
        if (fabsf(denom) > 1e-4f) {
            float t = dot(n, sub(board.p, origin)) / denom;
            if (t > 0 && t < 0.35f) {
                Vec3 hit = add(origin, mul(dir, t));
                Vec3 local = qrot(Quat{-board.q.x,-board.q.y,-board.q.z,board.q.w}, sub(hit, board.p));
                int row = (int)floorf((0.16f - local.y) / 0.07f);
                if (g_page==0 && row==1) g_board = !g_board;
                if (g_page==1 && row==1) g_fly = !g_fly;
                if (g_page==1 && row==2) g_speed = !g_speed;
                if (g_page==1 && row==3) g_noclip = !g_noclip;
                if (g_page==2 && row==1) SpawnCycleItem();
                if (g_page==2 && row==2) SpawnDo();
                if (g_page==3 && row==1) g_esp = !g_esp;
                LOGI("poke page=%s row=%d item=%s", kPages[g_page], row, SpawnCurrentItem());
            }
        }
    }
}
