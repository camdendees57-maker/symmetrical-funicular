#include "holdable_menu.h"
#include "xr_pose.h"
#include <android/log.h>
#include <cmath>
#include <cstring>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusHold", ##__VA_ARGS__)

struct Btn { const char* label; bool* flag; };
static bool g_fly=false, g_speed=false, g_noclip=false, g_esp=false;
static bool g_board=false;
static int g_page=0;
static bool g_prevTrig=false;
static float g_stickLatch=0;

static const char* kPages[] = { "HOME", "MOVE", "VISUAL", "MISC" };
static Btn kHome[] = { {"board lock", &g_board} };
static Btn kMove[] = { {"fly", &g_fly}, {"speed", &g_speed}, {"noclip", &g_noclip} };
static Btn kVis[]  = { {"esp", &g_esp} };
static Btn kMisc[] = { {"board lock", &g_board} };

static void PageButtons(Btn* b, int n, int& clicked){
    (void)b; (void)n; (void)clicked;
}

// ii-style: board glued to off-hand (left). poke with right trigger.
void HoldableOnPresent() {
    Hands h = ReadHands();
    bool hold = h.lGrip || g_board;
    if (!hold) return;

    if (h.rStickX > 0.7f && g_stickLatch <= 0) { g_page = (g_page+1)&3; g_stickLatch=1; }
    if (h.rStickX < -0.7f && g_stickLatch <= 0) { g_page = (g_page+3)&3; g_stickLatch=1; }
    if (fabsf(h.rStickX) < 0.3f) g_stickLatch = 0;

    // board pose = left hand + palm offset
    Pose board = h.left;
    if (board.valid) {
        Vec3 off = qrot(board.q, Vec3{0.08f, 0.02f, -0.12f});
        board.p = add(board.p, off);
    }

    bool poke = h.rTrig && !g_prevTrig;
    g_prevTrig = h.rTrig;

    // poke ray from right hand forward, hit test 3x3 grid
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
                // board 0.28 x 0.36, 4 rows x 1 col-ish pages of fat buttons
                int row = (int)floorf((0.16f - local.y) / 0.07f);
                if (row >= 0 && row < 4) {
                    Btn* set = kHome; int count=1;
                    if (g_page==1){ set=kMove; count=3; }
                    else if (g_page==2){ set=kVis; count=1; }
                    else if (g_page==3){ set=kMisc; count=1; }
                    if (row==0) {
                        LOGI("page %s", kPages[g_page]);
                    } else if (row-1 < count) {
                        *set[row-1].flag = !*set[row-1].flag;
                        LOGI("toggle %s -> %d", set[row-1].label, (int)*set[row-1].flag);
                    }
                }
            }
        }
    }

    static int once=0;
    if ((++once % 90)==0)
        LOGI("board page=%s fly=%d speed=%d hold=%d unity=%s", kPages[g_page], (int)g_fly,(int)g_speed,(int)hold, UNITY_VERSION_STR);
}
