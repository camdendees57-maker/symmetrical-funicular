cmake_minimum_required(VERSION 3.18)
project(holdmenu_unity6 C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(UNITY_VERSION "6000.3.12f1" CACHE STRING "Game Unity editor version")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui" CACHE PATH "Dear ImGui")
add_compile_definitions(UNITY_VERSION_STR="${UNITY_VERSION}")
add_library(holdmenu SHARED
  src/main.cpp
  src/holdable_menu.cpp
  src/imgui_vr.cpp
  src/imgui_panel.cpp
  src/xr_pose.cpp
  src/present_hook.cpp
  src/il2cpp_api.cpp
  src/spawn.cpp
)
target_include_directories(holdmenu PRIVATE src)
if(EXISTS "${IMGUI_DIR}/imgui.cpp")
  target_sources(holdmenu PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
  )
  target_include_directories(holdmenu PRIVATE ${IMGUI_DIR})
  target_compile_definitions(holdmenu PRIVATE HAS_IMGUI=1)
endif()
target_link_libraries(holdmenu android log EGL GLESv3 vulkan dl)
set_target_properties(holdmenu PROPERTIES OUTPUT_NAME "holdmenu")
