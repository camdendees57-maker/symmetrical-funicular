#include "xr_pose.h"
#include <dlfcn.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusXR", ##__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "TagtusXR", ##__VA_ARGS__)

// OVRPlugin C ABI used by Meta XR on Unity 6 Quest.
enum OvrNode { OVR_EyeLeft=0, OVR_EyeRight=1, OVR_EyeCenter=2, OVR_HandLeft=3, OVR_HandRight=4, OVR_Head=9 };
enum OvrStep { OVR_Render=0, OVR_Physics=1 };
enum OvrResult { OVR_Success=0 };

struct OvrVec3 { float x,y,z; };
struct OvrQuat { float x,y,z,w; };
struct OvrPosef { OvrQuat o; OvrVec3 p; };
struct OvrPoseStatef {
    OvrPosef pose;
    OvrVec3 velocity;
    OvrVec3 acceleration;
    OvrVec3 angularVelocity;
    OvrVec3 angularAcceleration;
    double time;
};

// ControllerState4 packed layout from OVRPlugin.cs (buttons/touches + 2 sticks + 2 index + 2 hand)
struct OvrControllerState4 {
    uint32_t ConnectedControllerTypes;
    uint32_t Buttons;
    uint32_t Touches;
    uint32_t NearTouches;
    float LIndexTrigger;
    float RIndexTrigger;
    float LHandTrigger;
    float RHandTrigger;
    float LThumbstick[2];
    float RThumbstick[2];
    uint8_t pad[64]; // newer fields after this; we only read the front
};

// button bits (OVRInput.RawButton)
static const uint32_t BTN_LIndexTrigger = 1u << 24;
static const uint32_t BTN_RIndexTrigger = 1u << 25;
static const uint32_t BTN_LHandTrigger  = 1u << 26;
static const uint32_t BTN_RHandTrigger  = 1u << 27;
static const uint32_t BTN_A = 1u << 0;
static const uint32_t BTN_B = 1u << 1;
static const uint32_t BTN_X = 1u << 8;
static const uint32_t BTN_Y = 1u << 9;

using FnGetNodePoseState3 = int (*)(int step, int node, OvrPoseStatef* out);
using FnGetNodePoseState2 = int (*)(int step, int node, OvrPoseStatef* out);
using FnGetNodePose       = OvrPosef (*)(int node, int step);
using FnGetControllerState4 = int (*)(uint32_t mask, OvrControllerState4* out);
using FnGetNodePresent    = int (*)(int node);

static FnGetNodePoseState3 g_pose3 = nullptr;
static FnGetNodePoseState2 g_pose2 = nullptr;
static FnGetNodePose       g_pose1 = nullptr;
static FnGetControllerState4 g_ctrl4 = nullptr;
static FnGetNodePresent    g_present = nullptr;
static bool g_ovr_ready = false;
static const char* g_backend = "none";

// OpenXR fallback: steal procs via xrGetInstanceProcAddr hook.
using PFN_xrVoid = void (*)();
using PFN_xrGetInstanceProcAddr = int (*)(uint64_t inst, const char* name, PFN_xrVoid* fn);
using PFN_xrLocateSpace = int (*)(uint64_t space, uint64_t base, int64_t time, void* loc);
using PFN_xrGetActionStateBoolean = int (*)(uint64_t session, const void* info, void* state);
using PFN_xrGetActionStateFloat = int (*)(uint64_t session, const void* info, void* state);
using PFN_xrGetActionStatePose = int (*)(uint64_t session, const void* info, void* state);

static PFN_xrGetInstanceProcAddr g_orig_gipa = nullptr;
static PFN_xrLocateSpace g_xrLocateSpace = nullptr;
static PFN_xrGetActionStateBoolean g_xrBool = nullptr;
static PFN_xrGetActionStateFloat g_xrFloat = nullptr;
static uint64_t g_xrInstance = 0;
static bool g_oxr_ready = false;

static int hooked_xrGetInstanceProcAddr(uint64_t inst, const char* name, PFN_xrVoid* fn) {
    int r = g_orig_gipa ? g_orig_gipa(inst, name, fn) : -1;
    if (inst) g_xrInstance = inst;
    if (name && fn && *fn) {
        if (!strcmp(name, "xrLocateSpace")) g_xrLocateSpace = (PFN_xrLocateSpace)*fn;
        if (!strcmp(name, "xrGetActionStateBoolean")) g_xrBool = (PFN_xrGetActionStateBoolean)*fn;
        if (!strcmp(name, "xrGetActionStateFloat")) g_xrFloat = (PFN_xrGetActionStateFloat)*fn;
        g_oxr_ready = g_xrLocateSpace != nullptr;
        if (g_oxr_ready) g_backend = "openxr-gipa";
    }
    return r;
}

static void* try_so(const char* name) {
    void* h = dlopen(name, RTLD_NOW);
    if (!h) h = dlopen(name, RTLD_LAZY);
    return h;
}

static void BindOvr() {
    const char* cands[] = {
        "libOVRPlugin.so",
        "libovrplatformloader.so",
        "libOculusXRPlugin.so",
        nullptr
    };
    void* h = nullptr;
    for (int i=0; cands[i] && !h; ++i) h = try_so(cands[i]);
    if (!h) {
        LOGI("no OVRPlugin so yet");
        return;
    }
    g_pose3 = (FnGetNodePoseState3)dlsym(h, "ovrp_GetNodePoseState3");
    g_pose2 = (FnGetNodePoseState2)dlsym(h, "ovrp_GetNodePoseState2");
    g_pose1 = (FnGetNodePose)dlsym(h, "ovrp_GetNodePose");
    g_ctrl4 = (FnGetControllerState4)dlsym(h, "ovrp_GetControllerState4");
    if (!g_ctrl4) g_ctrl4 = (FnGetControllerState4)dlsym(h, "ovrp_GetControllerState2");
    g_present = (FnGetNodePresent)dlsym(h, "ovrp_GetNodePresent");
    g_ovr_ready = g_pose3 || g_pose2 || g_pose1;
    if (g_ovr_ready) {
        g_backend = "ovrplugin";
        LOGI("OVR bind pose3=%p pose2=%p pose1=%p ctrl4=%p present=%p", (void*)g_pose3,(void*)g_pose2,(void*)g_pose1,(void*)g_ctrl4,(void*)g_present);
    }
}

static void BindOpenXrGipa() {
    void* oxr = try_so("libopenxr_loader.so");
    if (!oxr) oxr = try_so("libopenxr.so");
    if (!oxr) return;
    auto gipa = (PFN_xrGetInstanceProcAddr)dlsym(oxr, "xrGetInstanceProcAddr");
    if (!gipa || gipa == hooked_xrGetInstanceProcAddr) return;
    g_orig_gipa = gipa;
    // Export interposer symbol is in this .so; LD_PRELOAD / injector should bind xrGetInstanceProcAddr to ours.
    LOGI("openxr gipa @ %p", (void*)gipa);
}

extern "C" int xrGetInstanceProcAddr(uint64_t inst, const char* name, PFN_xrVoid* fn) {
    if (!g_orig_gipa) {
        void* oxr = try_so("libopenxr_loader.so");
        if (oxr) g_orig_gipa = (PFN_xrGetInstanceProcAddr)dlsym(oxr, "xrGetInstanceProcAddr");
        if (!g_orig_gipa) g_orig_gipa = (PFN_xrGetInstanceProcAddr)dlsym(RTLD_NEXT, "xrGetInstanceProcAddr");
    }
    return hooked_xrGetInstanceProcAddr(inst, name, fn);
}

static Pose FromOvr(const OvrPosef& p) {
    Pose o;
    // OVRPlugin is RH Y-up. Unity is LH Y-up. Flip Z on pos + quat z/w style convert.
    o.p = {p.p.x, p.p.y, -p.p.z};
    o.q = {-p.o.x, -p.o.y, p.o.z, p.o.w};
    o.valid = true;
    return o;
}

static bool ReadNode(int node, Pose& out) {
    OvrPoseStatef st{};
    if (g_pose3 && g_pose3(OVR_Render, node, &st) == OVR_Success) { out = FromOvr(st.pose); return true; }
    if (g_pose2 && g_pose2(OVR_Render, node, &st) == OVR_Success) { out = FromOvr(st.pose); return true; }
    if (g_pose1) { OvrPosef p = g_pose1(node, OVR_Render); out = FromOvr(p); return true; }
    return false;
}

Hands ReadHands() {
    Hands h{};
    h.backend = g_backend;
    if (g_ovr_ready) {
        ReadNode(OVR_HandLeft, h.left);
        ReadNode(OVR_HandRight, h.right);
        ReadNode(OVR_Head, h.head);
        if (!h.head.valid) ReadNode(OVR_EyeCenter, h.head);
        if (g_ctrl4) {
            OvrControllerState4 cs{};
            // mask 0xFFFFFFFF = all connected
            if (g_ctrl4(0xFFFFFFFFu, &cs) == OVR_Success || true) {
                h.lTrigVal = cs.LIndexTrigger;
                h.rTrigVal = cs.RIndexTrigger;
                h.lGripVal = cs.LHandTrigger;
                h.rGripVal = cs.RHandTrigger;
                h.lStickX = cs.LThumbstick[0];
                h.lStickY = cs.LThumbstick[1];
                h.rStickX = cs.RThumbstick[0];
                h.rStickY = cs.RThumbstick[1];
                h.lTrig = h.lTrigVal > 0.55f || (cs.Buttons & BTN_LIndexTrigger);
                h.rTrig = h.rTrigVal > 0.55f || (cs.Buttons & BTN_RIndexTrigger);
                h.lGrip = h.lGripVal > 0.55f || (cs.Buttons & BTN_LHandTrigger);
                h.rGrip = h.rGripVal > 0.55f || (cs.Buttons & BTN_RHandTrigger);
            }
        }
    }
    return h;
}

static void* resolver(void*) {
    for (int i=0;i<120;++i) {
        if (!g_ovr_ready) BindOvr();
        if (!g_orig_gipa) BindOpenXrGipa();
        if (g_ovr_ready) break;
        usleep(200*1000);
    }
    LOGI("xr resolve done backend=%s unity=%s", g_backend, UNITY_VERSION_STR);
    return nullptr;
}

void ResolveXrHooks() {
    pthread_t t; pthread_create(&t,nullptr,resolver,nullptr); pthread_detach(t);
}
