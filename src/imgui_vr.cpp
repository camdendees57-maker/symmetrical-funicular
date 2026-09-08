#include "imgui_vr.h"
#include <cmath>
#include <cstring>

struct VRState {
    int mode = IMGUIVR_FOLLOW_LOOK;
    float panel_w = 0.70f;
    float panel_h = 0.45f;
    float look_dist = 1.15f;
    Vec3 hand_off = {0.08f, 0.06f, -0.18f};
    Pose head{};
    Pose hand{};
    int inited = 0;
};
static VRState g;

static Vec3 nrm(Vec3 v) {
    float n = std::sqrt(dot(v, v));
    if (n < 1e-8f) return {0, 0, 1};
    return mul(v, 1.f / n);
}

static Quat qinv(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }

static void mtx_basis(float* m, Vec3 p, Vec3 right, Vec3 up, Vec3 fwd) {
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = right.x; m[1] = right.y; m[2] = right.z;
    m[4] = up.x;    m[5] = up.y;    m[6] = up.z;
    m[8] = fwd.x;   m[9] = fwd.y;   m[10]= fwd.z;
    m[12]= p.x;     m[13]= p.y;     m[14]= p.z; m[15]= 1;
}

void ImGuiVR_Init(float panel_w_m, float panel_h_m, float look_distance_m) {
    g = VRState{};
    g.inited = 1;
    if (panel_w_m > 0.05f) g.panel_w = panel_w_m;
    if (panel_h_m > 0.05f) g.panel_h = panel_h_m;
    if (look_distance_m > 0.2f) g.look_dist = look_distance_m;
}

void ImGuiVR_SetHead(const Pose& head) { g.head = head; if (!g.inited) ImGuiVR_Init(0.70f, 0.45f, 1.15f); }
void ImGuiVR_SetLeftHand(const Pose& hand) { g.hand = hand; }
void ImGuiVR_SetLookDistance(float meters) { if (meters > 0.2f) g.look_dist = meters; }
void ImGuiVR_SetHandOffset(Vec3 local_offset) { g.hand_off = local_offset; }
void ImGuiVR_SetFollowMode(int mode) {
    g.mode = (mode == IMGUIVR_FOLLOW_LEFT_HAND) ? IMGUIVR_FOLLOW_LEFT_HAND : IMGUIVR_FOLLOW_LOOK;
}
int ImGuiVR_GetFollowMode() { return g.mode; }

void ImGuiVR_BuildWorld(float out[16]) {
    if (!out) return;
    if (!g.inited) ImGuiVR_Init(0.70f, 0.45f, 1.15f);

    if (g.mode == IMGUIVR_FOLLOW_LEFT_HAND && g.hand.valid) {
        Vec3 pos = add(g.hand.p, qrot(g.hand.q, g.hand_off));
        Vec3 fwd = qrot(g.hand.q, {0, 0, -1});
        Vec3 up  = qrot(g.hand.q, {0, 1, 0});
        Vec3 right = nrm(cross(up, fwd));
        up = nrm(cross(fwd, right));
        mtx_basis(out, pos, right, up, fwd);
        return;
    }

    Pose hd = g.head;
    if (!hd.valid) { hd.p = {0, 1.6f, 0}; hd.q = {0, 0, 0, 1}; hd.valid = true; }
    Vec3 fwd = qrot(hd.q, {0, 0, -1});
    fwd.y *= 0.15f;
    fwd = nrm(fwd);
    Vec3 pos = add(hd.p, mul(fwd, g.look_dist));
    Vec3 to_head = nrm(sub(hd.p, pos));
    Vec3 world_up = {0, 1, 0};
    Vec3 right = nrm(cross(world_up, to_head));
    if (dot(right, right) < 1e-6f) right = {1, 0, 0};
    Vec3 up = nrm(cross(to_head, right));
    mtx_basis(out, pos, right, up, to_head);
}

bool ImGuiVR_TryToggleFromPoke(const Pose& board, const Pose& right_hand) {
    if (!board.valid || !right_hand.valid) return false;
    Vec3 origin = right_hand.p;
    Vec3 dir = qrot(right_hand.q, {0, 0, -1});
    Vec3 n = qrot(board.q, {0, 0, 1});
    float denom = dot(n, dir);
    if (fabsf(denom) < 1e-4f) return false;
    float t = dot(n, sub(board.p, origin)) / denom;
    if (t <= 0.f || t > 0.35f) return false;
    Vec3 hit = add(origin, mul(dir, t));
    Vec3 local = qrot(qinv(board.q), sub(hit, board.p));
    /* button strip near top of panel */
    bool on_btn = (local.y > 0.04f && local.y < 0.16f && fabsf(local.x) < 0.18f);
    if (!on_btn) return false;
    if (g.mode == IMGUIVR_FOLLOW_LOOK) {
        if (g.hand.valid) g.mode = IMGUIVR_FOLLOW_LEFT_HAND;
    } else {
        g.mode = IMGUIVR_FOLLOW_LOOK;
    }
    return true;
}
