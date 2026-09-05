#include "il2cpp_api.h"
#include <android/log.h>
#include <cstring>
#include <cctype>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusSpawn", ##__VA_ARGS__)

static const char* kItems[] = {
    "Flashlight","Backpack","Shotgun","Grappler","Hoverboard","Rocket","LootItem","MineItem","Weapon"
};
static int g_item=0;
void SpawnCycleItem(){ g_item=(g_item+1)%9; LOGI("item %s", kItems[g_item]); }
const char* SpawnCurrentItem(){ return kItems[g_item]; }

static bool Has(const char* s, const char* n){
    if(!s||!n) return false;
    size_t ns=strlen(s), nn=strlen(n);
    if(nn>ns) return false;
    for(size_t i=0;i+nn<=ns;i++){
        size_t j=0;
        for(;j<nn;j++) if(tolower((unsigned char)s[i+j])!=tolower((unsigned char)n[j])) break;
        if(j==nn) return true;
    }
    return false;
}
static bool ClassHot(const char* ns, const char* name){
    if (Has(ns,"UnityEngine")||Has(ns,"System")||Has(ns,"Photon.Chat")||Has(ns,"TMPro")||Has(ns,"Oculus")||Has(ns,"Meta.XR")||Has(ns,"XR.Interaction")) return false;
    return Has(name,"Spawn")||Has(name,"Item")||Has(name,"Loot")||Has(name,"Invent")||Has(name,"Prefab");
}
static bool MethodHot(const char* name){
    if (!name) return false;
    if (Has(name,"get_")||Has(name,"set_")||Has(name,".ctor")||Has(name,"add_")||Has(name,"remove_")) return false;
    return Has(name,"Spawn")||Has(name,"Give")||Has(name,"AddItem")||Has(name,"Instantiate")||Has(name,"CreateItem")||Has(name,"MakeItem");
}

struct Hit { const MethodInfo* mi; char c[64]; char m[64]; int argc; };
static Hit g_hits[64];
static int g_nhit=0;
static bool g_scanned=false;

static void OnMethod(Il2CppClass*, const MethodInfo* mi, const char* cname, const char* mname, int argc, void*){
    if (!MethodHot(mname)) return;
    if (g_nhit>=64) return;
    if (argc>2) return;
    Hit& h=g_hits[g_nhit++];
    h.mi=mi; h.argc=argc;
    strncpy(h.c, cname?cname:"?", 63); h.c[63]=0;
    strncpy(h.m, mname?mname:"?", 63); h.m[63]=0;
    LOGI("scan %s.%s argc=%d", h.c, h.m, argc);
}
static void OnClass(Il2CppClass* k, const char* ns, const char* name, void*){
    if (!ClassHot(ns,name)) return;
    Il2CppForEachMethod(k, OnMethod, nullptr);
}

static void Scan(){
    if (g_scanned) return;
    if (!Il2CppReady()) return;
    g_nhit=0;
    Il2CppForEachClass(OnClass, nullptr);
    g_scanned=true;
    LOGI("scan done hits=%d", g_nhit);
}

bool SpawnDo(){
    Scan();
    if (!g_nhit){ LOGI("no spawn hits yet"); return false; }
    void* s=Il2CppStr(kItems[g_item]);
    void* args1[1]={s};
    int ok=0;
    for(int i=0;i<g_nhit;i++){
        void** args = g_hits[i].argc==1 ? args1 : nullptr;
        void* r=Il2CppInvokeMi(g_hits[i].mi, nullptr, args);
        if(r){ LOGI("fired %s.%s", g_hits[i].c, g_hits[i].m); ok++; }
    }
    LOGI("spawn fire ok=%d / %d item=%s", ok, g_nhit, kItems[g_item]);
    return ok>0;
}
