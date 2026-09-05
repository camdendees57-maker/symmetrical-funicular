#include "il2cpp_api.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusSpawn", ##__VA_ARGS__)

// From Float Company global-metadata.dat v31:
//   Assets/Mod Menu/Scripts/ItemSpawn.cs  fields: spawnSpot
//   LootSpawnFix.Spawn / Respawn / spawners
//   Photon PUN Instantiation
// Prefab names pulled from script types in the same metadata dump.

static const char* kItems[] = {
    "Flashlight", "Backpack", "Shotgun", "Grappler", "Hoverboard", "Rocket",
    "LootItem", "MineItem", "Weapon"
};
static int g_item = 0;

void SpawnCycleItem() {
    g_item = (g_item + 1) % 9;
    LOGI("item %s", kItems[g_item]);
}
const char* SpawnCurrentItem() { return kItems[g_item]; }

static bool Try(const char* asmName, const char* ns, const char* clazz, const char* method, int argc, void* inst, void** args) {
    void* r = Il2CppInvoke(asmName, ns, clazz, method, argc, inst, args);
    if (r) { LOGI("hit %s.%s", clazz, method); return true; }
    return false;
}

bool SpawnDo() {
    if (!Il2CppReady()) { LOGI("il2cpp cold"); return false; }
    const char* item = kItems[g_item];
    void* name = Il2CppStr(item);
    void* args1[1] = { name };

    // built-in mod menu spawner first
    if (Try("Assembly-CSharp", "", "ItemSpawn", "Spawn", 0, nullptr, nullptr)) return true;
    if (Try("Assembly-CSharp", "", "ItemSpawn", "SpawnOne", 0, nullptr, nullptr)) return true;
    if (Try("Assembly-CSharp", "", "ItemSpawn", "Spawn", 1, nullptr, args1)) return true;

    if (Try("Assembly-CSharp", "", "LootSpawnFix", "Spawn", 0, nullptr, nullptr)) return true;
    if (Try("Assembly-CSharp", "", "LootItem", "Spawn", 0, nullptr, nullptr)) return true;

    // Photon PUN — 4-arg instantiate(string, Vector3, Quaternion, byte) needs valuetype packing;
    // try the string-only overloads first if they exist.
    if (Try("PhotonUnityNetworking", "Photon.Pun", "PhotonNetwork", "Instantiate", 1, nullptr, args1)) return true;
    if (Try("Assembly-CSharp", "Photon.Pun", "PhotonNetwork", "Instantiate", 1, nullptr, args1)) return true;

    LOGI("spawn miss item=%s — ItemSpawn exists, method name not 0/1-arg Spawn", item);
    return false;
}
