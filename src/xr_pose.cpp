#include "xr_pose.h"
#include <dlfcn.h>
#include <cstring>
// Unity 6 + Meta XR: poses come from OVRPlugin / OpenXR at runtime.
// Injector should patch these later to real ovrp_GetNodePoseState3 / xrLocateViews.
// Until hooked, returns last cached / empty so the board logic still compiles.
static Hands g_cache{};
Hands ReadHands() { return g_cache; }
void HoldMenu_SetHands(const Hands& h){ g_cache = h; }
