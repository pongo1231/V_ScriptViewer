cmake_minimum_required(VERSION 3.12)

project(ScriptViewer)

add_subdirectory(vendor/shv)
#add_subdirectory(vendor/imgui)
add_subdirectory(vendor/minhook)

file(GLOB_RECURSE src ScriptViewer/*.cpp)
add_library(ScriptViewer MODULE ${src})

set_property(TARGET ScriptViewer PROPERTY CXX_STANDARD 20)

set(IMGUI_PATH "vendor/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp ${IMGUI_PATH}/backends/imgui_impl_dx11.cpp ${IMGUI_PATH}/backends/imgui_impl_win32.cpp) 
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_PATH} ${IMGUI_PATH}/backends)

if (RELOADABLE)
    target_compile_definitions(ScriptViewer PRIVATE RELOADABLE)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(ScriptViewer PRIVATE -DUNICODE -D_UNICODE -DNOMINMAX /Zi)
    target_link_options(ScriptViewer PRIVATE /DEBUG /OPT:REF /OPT:ICF)
else()
    target_compile_options(ScriptViewer PRIVATE -municode -fpermissive)
    target_link_options(ScriptViewer PRIVATE -static)
endif()

target_precompile_headers(ScriptViewer PUBLIC ScriptViewer/stdafx.h)

target_include_directories(ScriptViewer PUBLIC ScriptViewer)

target_link_libraries(ScriptViewer PRIVATE shv minhook d3dcompiler dwmapi ImGui)

set_target_properties(ScriptViewer PROPERTIES SUFFIX ".asi")