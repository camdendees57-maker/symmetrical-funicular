# symmetrical-funicular

Quest Tools menu `.so`. Dear ImGui, arm64, OpenGL ES 3.

Actions builds `libaxiommenu.so`. In Quest Tools, pick that file as the menu `.so` and patch.

Loaded libs run a constructor, hook `eglSwapBuffers`, and draw the Axiom window.

OpenGL ES games only. Vulkan titles load the lib and log `eglSwapBuffers not found`. They will not draw this window.

Log tag: `AxiomMenu`.
