#pragma once
#include <cstddef>
struct Il2CppDomain; struct Il2CppAssembly; struct Il2CppImage;
struct Il2CppClass; struct MethodInfo; struct Il2CppObject; struct Il2CppString;
bool Il2CppReady();
void Il2CppBind();
void* Il2CppInvoke(const char* asmName, const char* namespaze, const char* clazz,
                   const char* method, int argc, void* instance, void** args);
Il2CppString* Il2CppStr(const char* utf8);
