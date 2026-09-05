#include "il2cpp_api.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusSpawn", ##__VA_ARGS__)

// Class/method names must match dump.cs. These are the usual AC-clone slots.
// Swap after you dump Assembly-CSharp for Float Company.
struct SpawnTarget {
    const char* asmName;
    const char* ns;
    const char* clazz;
    const char* method;
    int argc;
    const char* item;
};

static SpawnTarget kTargets[] = {
    {"Assembly-CSharp", "", "ItemSpawner", "Spawn", 1, "flashlight"},
    {"Assembly-CSharp", "", "ItemSpawner", "SpawnItem", 1, "flashlight"},
    {"Assembly-CSharp", "", "SpawnManager", "Spawn", 1, "flashlight"},
    {"Assembly-CSharp", "", "PlayerInventory", "AddItem", 1, "flashlight"},
    {"Assembly-CSharp", "", "Inventory", "Give", 1, "flashlight"},
};

static const char* kItems[] = {
    "flashlight", "backpack", "shotgun", "grappler", "hoverboard", "rocket"
};
static int g_item = 0;

void SpawnCycleItem() {
    g_item = (g_item + 1) % 6;
    LOGI("spawn item slot %s", kItems[g_item]);
}

const char* SpawnCurrentItem() { return kItems[g_item]; }

bool SpawnDo() {
    if (!Il2CppReady()) { LOGI("il2cpp not ready"); return false; }
    const char* item = kItems[g_item];
    void* s = Il2CppStr(item);
    void* args[1] = { s };
    for (auto& t : kTargets) {
        args[0] = Il2CppStr(t.item[0] ? item : t.item);
        void* r = Il2CppInvoke(t.asmName, t.ns, t.clazz, t.method, t.argc, nullptr, args);
        if (r) { LOGI("spawn hit %s.%s(%s)", t.clazz, t.method, item); return true; }
    }
    LOGI("no spawn method hit — drop dump.cs class names");
    return false;
}
