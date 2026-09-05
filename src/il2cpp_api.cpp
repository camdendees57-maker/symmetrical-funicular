#include "il2cpp_api.h"
#include "Il2Cpp-Headers.hpp"
#include <dlfcn.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TagtusIL2", ##__VA_ARGS__)

using FnDomainGet = Il2CppDomain*(*)();
using FnAsmOpen = Il2CppAssembly*(*)(Il2CppDomain*, const char*);
using FnAsmImage = Il2CppImage*(*)(const Il2CppAssembly*);
using FnClassFromName = Il2CppClass*(*)(const Il2CppImage*, const char*, const char*);
using FnMethodFromName = const MethodInfo*(*)(Il2CppClass*, const char*, int);
using FnInvoke = Il2CppObject*(*)(const MethodInfo*, void*, void**, void**);
using FnAttach = void*(*)(Il2CppDomain*);
using FnCurrent = void*(*)();
using FnStrNew = Il2CppString*(*)(const char*);
using FnObjNew = Il2CppObject*(*)(Il2CppClass*);

static FnDomainGet domain_get;
static FnAsmOpen asm_open;
static FnAsmImage asm_image;
static FnClassFromName class_from_name;
static FnMethodFromName method_from_name;
static FnInvoke runtime_invoke;
static FnAttach thread_attach;
static FnCurrent thread_current;
static FnStrNew string_new;
static FnObjNew object_new;
static bool g_ok=false;

static void* so() {
    void* h = dlopen("libil2cpp.so", RTLD_NOW);
    if (!h) h = dlopen("libil2cpp.so", RTLD_LAZY);
    return h;
}

void Il2CppBind() {
    void* h = so();
    if (!h) { LOGI("libil2cpp missing"); return; }
#define BIND(fn, sym) fn = (decltype(fn))dlsym(h, sym)
    BIND(domain_get, symbol_il2cpp_domain_get);
    BIND(asm_open, symbol_il2cpp_domain_assembly_open);
    BIND(asm_image, symbol_il2cpp_assembly_get_image);
    BIND(class_from_name, symbol_il2cpp_class_from_name);
    BIND(method_from_name, symbol_il2cpp_class_get_method_from_name);
    BIND(runtime_invoke, symbol_il2cpp_runtime_invoke);
    BIND(thread_attach, symbol_il2cpp_thread_attach);
    BIND(thread_current, symbol_il2cpp_thread_current);
    BIND(string_new, symbol_il2cpp_string_new);
    BIND(object_new, symbol_il2cpp_object_new);
#undef BIND
    g_ok = domain_get && asm_open && class_from_name && method_from_name && runtime_invoke;
    LOGI("il2cpp bind ok=%d domain=%p invoke=%p unity=%s", (int)g_ok, (void*)domain_get, (void*)runtime_invoke, UNITY_VERSION_STR);
}

bool Il2CppReady() { return g_ok; }

Il2CppString* Il2CppStr(const char* utf8) {
    return string_new ? string_new(utf8) : nullptr;
}

void* Il2CppInvoke(const char* asmName, const char* namespaze, const char* clazz,
                   const char* method, int argc, void* instance, void** args) {
    if (!g_ok) return nullptr;
    auto dom = domain_get();
    if (!dom) return nullptr;
    if (thread_attach && !thread_current()) thread_attach(dom);
    auto asmbl = asm_open(dom, asmName);
    if (!asmbl) return nullptr;
    auto img = asm_image(asmbl);
    if (!img) return nullptr;
    auto klass = class_from_name(img, namespaze ? namespaze : "", clazz);
    if (!klass) return nullptr;
    auto mi = method_from_name(klass, method, argc);
    if (!mi) return nullptr;
    void* exc = nullptr;
    auto ret = runtime_invoke(mi, instance, args, &exc);
    if (exc) { LOGI("invoke exc %s.%s", clazz, method); return nullptr; }
    LOGI("invoke ok %s::%s", clazz, method);
    return ret;
}
