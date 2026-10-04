cmake_minimum_required(VERSION 3.22.1)
project(axiommenu LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT IMGUI_DIR)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
endif()
if(NOT HOOK_DIR)
    set(HOOK_DIR "${CMAKE_SOURCE_DIR}/third_party/And64InlineHook")
endif()

add_library(axiommenu SHARED
    src/menu.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${HOOK_DIR}/And64InlineHook.cpp
)

target_include_directories(axiommenu PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${HOOK_DIR}
)

target_compile_definitions(axiommenu PRIVATE
    IMGUI_IMPL_OPENGL_ES3
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

target_link_libraries(axiommenu
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles-lib}
)
