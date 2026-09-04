# symmetrical-funicular

ARM64 ImGui native overlay for Unity IL2CPP Android / Quest.

Set UNITY_VERSION in CMakeLists.txt and .github/workflows/build-arm64.yml to the game editor version (example 2021.3.33f1 / 2022.3.20f1). Mismatched Unity NDK / GLES backend is why a generic imgui.so dies on load or never draws.

## Build
- GitHub Actions: Actions tab → build-arm64 → artifact `libimgui.so` under `lib/arm64-v8a/`
- Local: Android NDK r23+ matching that Unity embed NDK

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DUNITY_VERSION=2021.3.33f1
cmake --build build
```

Drop `libimgui.so` next to the game libs or inject via your existing loader.
