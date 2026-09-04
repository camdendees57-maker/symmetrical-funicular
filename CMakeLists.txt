cmake_minimum_required(VERSION 3.18)
project(imgui_unity_arm64 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT DEFINED UNITY_VERSION)
  set(UNITY_VERSION "2021.3.33f1" CACHE STRING "Game Unity editor version")
endif()

add_compile_definitions(UNITY_VERSION_STR="${UNITY_VERSION}")
add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)

# Pull imgui as submodule-less single-file drop: put Dear ImGui sources in third_party/imgui
file(GLOB IMGUI_SRC
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
  third_party/imgui/imgui_demo.cpp
  third_party/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui SHARED
  src/main.cpp
  src/hook.cpp
  ${IMGUI_SRC}
)

target_include_directories(imgui PRIVATE
  src
  third_party/imgui
  third_party/imgui/backends
)

target_link_libraries(imgui
  android
  log
  EGL
  GLESv3
  dl
)

set_target_properties(imgui PROPERTIES
  OUTPUT_NAME "imgui"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/arm64-v8a"
)
