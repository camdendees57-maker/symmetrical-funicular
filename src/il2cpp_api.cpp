#include "il2cpp_api.h"
#include "Il2Cpp-Headers.hpp"
#include "Il2CppMethodNames.hpp"
#include <dlfcn.h>
#include <android/log.h>
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
using FnGetAsms = Il2CppAssembly**(*)(const Il2CppDomain*, size_t*);
using FnImgClassCount = size_t(*)(const Il2CppImage*);
using FnImgClass = Il2CppClass*(*)(const Il2CppImage*, size_t);
using FnClassName = const char*(*)(Il2CppClass*);
using FnClassNs = const char*(*)(Il2CppClass*);
using FnClassMethods = const MethodInfo*(*)(Il2CppClass*, void**);
using FnMethodName = const char*(*)(const MethodInfo*);
using FnMethodArgc = int(*)(const MethodInfo*);
using FnForEach = void(*)(void(*)(Il2CppClass*,void*), void*);

static FnDomainGet domain_get;
static FnAsmOpen asm_open;
static FnAsmImage asm_image;
static FnClassFromName class_from_name;
static FnMethodFromName method_from_name;
static FnInvoke runtime_invoke;
static FnAttach thread_attach;
static FnCurrent thread_current;
static FnStrNew string_new;
static FnGetAsms domain_asms;
static FnImgClassCount img_count;
static FnImgClass img_class;
static FnClassName class_name;
static FnClassNs class_ns;
static FnClassMethods class_methods;
static FnMethodName method_name;
static FnMethodArgc method_argc;
static FnForEach class_for_each;
static bool g_ok=false;

void Il2CppBind() {
    void* h = dlopen("libil2cpp.so", RTLD_NOW);
    if (!h) h = dlopen("libil2cpp.so", RTLD_LAZY);
    if (!h) return;
#define BIND(fn, SYM) fn = (decltype(fn))dlsym(h, SYM)
    BIND(domain_get,        symbol_il2cpp_domain_get);
    BIND(asm_open,          symbol_il2cpp_domain_assembly_open);
    BIND(asm_image,         symbol_il2cpp_assembly_get_image);
    BIND(class_from_name,   symbol_il2cpp_class_from_name);
    BIND(method_from_name,  symbol_il2cpp_class_get_method_from_name);
    BIND(runtime_invoke,    symbol_il2cpp_runtime_invoke);
    BIND(thread_attach,     symbol_il2cpp_thread_attach);
    BIND(thread_current,    symbol_il2cpp_thread_current);
    BIND(string_new,        symbol_il2cpp_string_new);
    BIND(domain_asms,       symbol_il2cpp_domain_get_assemblies);
    BIND(img_count,         symbol_il2cpp_image_get_class_count);
    BIND(img_class,         symbol_il2cpp_image_get_class);
    BIND(class_name,        symbol_il2cpp_class_get_name);
    BIND(class_ns,          symbol_il2cpp_class_get_namespace);
    BIND(class_methods,     symbol_il2cpp_class_get_methods);
    BIND(method_name,       symbol_il2cpp_method_get_name);
    BIND(method_argc,       symbol_il2cpp_method_get_param_count);
    BIND(class_for_each,    symbol_il2cpp_class_for_each);
    if (!domain_get)     BIND(domain_get, BNM_IL2CPP_API_il2cpp_domain_get);
    if (!runtime_invoke) BIND(runtime_invoke, BNM_IL2CPP_API_il2cpp_runtime_invoke);
    if (!string_new)     BIND(string_new, BNM_IL2CPP_API_il2cpp_string_new);
    if (!asm_open)       BIND(asm_open, BNM_IL2CPP_API_il2cpp_domain_assembly_open);
    if (!class_from_name)BIND(class_from_name, BNM_IL2CPP_API_il2cpp_class_from_name);
#undef BIND
    g_ok = domain_get && runtime_invoke && class_name && class_methods;
    LOGI("symbol-header bind ok=%d", (int)g_ok);
}
bool Il2CppReady(){ return g_ok; }
Il2CppString* Il2CppStr(const char* s){ return string_new ? string_new(s) : nullptr; }
void Il2CppAttach(){
    if (!domain_get) return;
    auto d=domain_get();
    if (thread_attach && d && (!thread_current || !thread_current())) thread_attach(d);
}
const char* Il2CppClassName(Il2CppClass* k){ return (k && class_name) ? class_name(k) : ""; }
const char* Il2CppClassNs(Il2CppClass* k){ return (k && class_ns) ? class_ns(k) : ""; }
const char* Il2CppMethodName(const MethodInfo* m){ return (m && method_name) ? method_name(m) : ""; }
int Il2CppMethodArgc(const MethodInfo* m){ return (m && method_argc) ? method_argc(m) : -1; }
void* Il2CppInvokeMi(const MethodInfo* mi, void* inst, void** args){
    if (!runtime_invoke || !mi) return nullptr;
    Il2CppAttach();
    void* exc=nullptr;
    auto r=runtime_invoke(mi, inst, args, &exc);
    return exc ? nullptr : r;
}
void* Il2CppInvoke(const char* asmName, const char* ns, const char* clazz, const char* method, int argc, void* inst, void** args){
    if (!g_ok) return nullptr;
    Il2CppAttach();
    auto d=domain_get(); if(!d) return nullptr;
    auto a=asm_open(d, asmName); if(!a) return nullptr;
    auto img=asm_image(a); if(!img) return nullptr;
    auto k=class_from_name(img, ns?ns:"", clazz); if(!k) return nullptr;
    auto mi=method_from_name(k, method, argc); if(!mi) return nullptr;
    return Il2CppInvokeMi(mi, inst, args);
}
struct Wrap { ClassFn fn; void* user; };
static void EachThunk(Il2CppClass* k, void* u){
    auto* w=(Wrap*)u;
    w->fn(k, Il2CppClassNs(k), Il2CppClassName(k), w->user);
}
void Il2CppForEachClass(ClassFn fn, void* user){
    if (!g_ok) return;
    Il2CppAttach();
    if (class_for_each) { Wrap w{fn,user}; class_for_each(EachThunk, &w); return; }
    if (!domain_asms || !img_count || !img_class) return;
    size_t n=0;
    auto asms=domain_asms(domain_get(), &n);
    if (!asms) return;
    for (size_t i=0;i<n;i++){
        auto img=asm_image(asms[i]); if(!img) continue;
        size_t cc=img_count(img);
        for (size_t c=0;c<cc;c++){
            auto k=img_class(img,c);
            if (k) fn(k, Il2CppClassNs(k), Il2CppClassName(k), user);
        }
    }
}
void Il2CppForEachMethod(Il2CppClass* klass, MethodFn fn, void* user){
    if (!klass || !class_methods || !method_name) return;
    void* iter=nullptr;
    const MethodInfo* mi;
    const char* cname=Il2CppClassName(klass);
    while ((mi=class_methods(klass,&iter)))
        fn(klass, mi, cname, method_name(mi), method_argc?method_argc(mi):-1, user);
}
