cmake_minimum_required(VERSION 3.12)

project(ScriptViewer)

add_subdirectory(vendor/shv)
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/minhook)

file(GLOB_RECURSE src ScriptViewer/*.cpp)
add_library(ScriptViewer MODULE ${src})

set_property(TARGET ScriptViewer PROPERTY CXX_STANDARD 20)

if (RELOADABLE)
    target_compile_definitions(ScriptViewer PRIVATE RELOADABLE)
endif ()

target_compile_options(ScriptViewer PRIVATE -municode -fpermissive)

target_precompile_headers(ScriptViewer PUBLIC ScriptViewer/stdafx.h)

target_include_directories(ScriptViewer PUBLIC ScriptViewer)

target_link_libraries(ScriptViewer PRIVATE shv IMGUI minhook d3dcompiler dwmapi)
target_link_options(ScriptViewer PRIVATE -static)

set_target_properties(ScriptViewer PROPERTIES SUFFIX ".asi")