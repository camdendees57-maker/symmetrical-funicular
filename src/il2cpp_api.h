#pragma once
struct Il2CppDomain; struct Il2CppAssembly; struct Il2CppImage;
struct Il2CppClass; struct MethodInfo; struct Il2CppObject; struct Il2CppString;
bool Il2CppReady();
void Il2CppBind();
void* Il2CppInvoke(const char* asmName, const char* namespaze, const char* clazz,
                   const char* method, int argc, void* instance, void** args);
Il2CppString* Il2CppStr(const char* utf8);
void Il2CppAttach();
using ClassFn = void (*)(Il2CppClass* klass, const char* ns, const char* name, void* user);
using MethodFn = void (*)(Il2CppClass* klass, const MethodInfo* mi, const char* cname, const char* mname, int argc, void* user);
void Il2CppForEachClass(ClassFn fn, void* user);
void Il2CppForEachMethod(Il2CppClass* klass, MethodFn fn, void* user);
const char* Il2CppClassName(Il2CppClass* k);
const char* Il2CppClassNs(Il2CppClass* k);
const char* Il2CppMethodName(const MethodInfo* m);
int Il2CppMethodArgc(const MethodInfo* m);
void* Il2CppInvokeMi(const MethodInfo* mi, void* instance, void** args);
