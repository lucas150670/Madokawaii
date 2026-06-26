cmake_minimum_required(VERSION 3.31)
project(Madokawaii)

# configured with vcpkg
find_package(RapidJSON CONFIG REQUIRED)
find_package(yaml CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(libzippp CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")

if(NOT DEFINED implementer)
    set(implementer "RAYLIB")
endif()

message(STATUS "implementer: ${implementer}")

set(SRC_APP
        src/app/main.cpp
        src/app/main_menu.cpp
        src/app/chart.cpp
        src/app/ending.cpp
        src/app/app_config.cpp
        src/app/line_update.cpp
        src/app/line_renderer.cpp
        src/app/res_pack.cpp
        src/app/note_render.cpp
        src/app/hit_sound_fx.cpp
        src/app/hit_visual_fx.cpp
        src/app/epilepsy_warning.cpp
        src/app/pec.cpp)

set(SRC_IMP_RAYLIB
        src/platform/raylib/raylib_graphics.cpp
        src/platform/raylib/raylib_entry.cpp
        src/platform/raylib/raylib_core.cpp
        src/platform/raylib/raylib_log.cpp
        src/platform/raylib/raylib_audio.cpp
        src/platform/raylib/raylib_fonts.cpp
        src/platform/raylib/raylib_texture.cpp
        src/platform/raylib/raylib_gui.cpp
)

set(SRC_IMP_DIRECT2D
        src/platform/direct2d/direct2d_platform.cpp
        src/platform/direct2d/direct2d_graphics.cpp
        src/platform/direct2d/direct2d_entry.cpp
        src/platform/direct2d/direct2d_core.cpp
        src/platform/direct2d/direct2d_log.cpp
        src/platform/direct2d/direct2d_fonts.cpp
        src/platform/direct2d/direct2d_texture.cpp
        src/platform/direct2d/direct2d_gui.cpp
        # Audio stays on raylib for now.
        src/platform/raylib/raylib_audio.cpp
)

if(implementer STREQUAL "RAYLIB")
    find_package(raylib CONFIG REQUIRED)
    add_executable(Madokawaii
            ${SRC_APP}
            ${SRC_IMP_RAYLIB})
    if(MSVC)
        target_compile_options(Madokawaii PRIVATE /utf-8)
    endif()
    target_link_libraries(Madokawaii PRIVATE raylib RapidJSON rapidjson yaml libzip::zip libzippp::libzippp)
elseif(implementer STREQUAL "DIRECT2D")
    if(NOT WIN32)
        message(FATAL_ERROR "DIRECT2D implementer is only available on Windows")
    endif()
    find_package(raylib CONFIG REQUIRED)
    add_executable(Madokawaii
            ${SRC_APP}
            ${SRC_IMP_DIRECT2D})
    if(MSVC)
        target_compile_options(Madokawaii PRIVATE /utf-8)
    endif()
    target_compile_definitions(Madokawaii PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(Madokawaii PRIVATE
            raylib
            d2d1
            dwrite
            windowscodecs
            d3d11
            dxgi
            dxguid
            ole32
            uuid
            version
            comdlg32
            shell32
            user32
            gdi32
            RapidJSON
            rapidjson
            yaml
            libzip::zip
            libzippp::libzippp)
else()
    message(FATAL_ERROR "unknown implementer: ${implementer}. check CMakeLists.txt")
endif()

target_include_directories(Madokawaii PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
