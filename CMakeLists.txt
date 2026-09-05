cmake_minimum_required(VERSION 3.18)
project(holdmenu_unity6 C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(UNITY_VERSION "6000.0.66f2" CACHE STRING "Game Unity editor version")
add_compile_definitions(UNITY_VERSION_STR="${UNITY_VERSION}")
add_library(holdmenu SHARED
  src/main.cpp
  src/holdable_menu.cpp
  src/xr_pose.cpp
  src/present_hook.cpp
  src/il2cpp_api.cpp
  src/spawn.cpp
)
target_include_directories(holdmenu PRIVATE src)
target_link_libraries(holdmenu android log EGL GLESv3 vulkan dl)
set_target_properties(holdmenu PROPERTIES OUTPUT_NAME "holdmenu")
