cmake_minimum_required(VERSION 3.31)
project(Madokawaii)

# configured with vcpkg
find_package(RapidJSON CONFIG REQUIRED)
find_package(yaml CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(libzippp CONFIG REQUIRED)
find_path(FAST_IO_INCLUDE_DIR fast_io.h REQUIRED)

set(CMAKE_CXX_STANDARD 20)

if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

if(NOT DEFINED implementer)
    set(implementer "RAYLIB")
endif()

string(TOUPPER "${implementer}" implementer_upper)

if(NOT DEFINED audio_implementer OR audio_implementer STREQUAL "")
    if(implementer_upper STREQUAL "RAYLIB")
        set(audio_implementer "RAYLIB")
    else()
        set(audio_implementer "FMOD")
    endif()
endif()

string(TOUPPER "${audio_implementer}" audio_implementer_upper)

message(STATUS "graphics implementer: ${implementer_upper}")
message(STATUS "audio implementer: ${audio_implementer_upper}")

set(MADOKAWAII_RENDER_BACKENDS RAYLIB DIRECT2D D3D11ON12 GDIPLUS)
set(MADOKAWAII_AUDIO_BACKENDS RAYLIB FMOD CRI)

if(NOT implementer_upper IN_LIST MADOKAWAII_RENDER_BACKENDS)
    message(FATAL_ERROR "unknown implementer: ${implementer}. Supported: ${MADOKAWAII_RENDER_BACKENDS}")
endif()

if(NOT audio_implementer_upper IN_LIST MADOKAWAII_AUDIO_BACKENDS)
    message(FATAL_ERROR "unknown audio_implementer: ${audio_implementer}. Supported: ${MADOKAWAII_AUDIO_BACKENDS}")
endif()

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
        src/app/audio_engine_credit.cpp
        src/app/epilepsy_warning.cpp
        src/app/pec.cpp)

set(SRC_PLATFORM_RAYLIB
        src/platform/raylib/raylib_runtime.cpp
        src/platform/raylib/raylib_graphics.cpp
        src/platform/raylib/raylib_core.cpp
        src/platform/raylib/raylib_log.cpp
        src/platform/raylib/raylib_fonts.cpp
        src/platform/raylib/raylib_texture.cpp
        src/platform/raylib/raylib_gui.cpp)

set(SRC_ENTRY_RAYLIB
        src/platform/raylib/raylib_entry.cpp)

set(SRC_PLATFORM_DIRECT2D
        src/platform/direct2d/direct2d_platform.cpp
        src/platform/direct2d/direct2d_graphics.cpp
        src/platform/direct2d/direct2d_core.cpp
        src/platform/direct2d/direct2d_log.cpp
        src/platform/direct2d/direct2d_fonts.cpp
        src/platform/direct2d/direct2d_texture.cpp
        src/platform/direct2d/direct2d_gui.cpp)

set(SRC_ENTRY_DIRECT2D
        src/platform/direct2d/direct2d_entry.cpp)

set(SRC_PLATFORM_D3D11ON12
        src/platform/d3d11on12/d3d11on12_platform.cpp
        ${SRC_PLATFORM_DIRECT2D})

set(SRC_ENTRY_D3D11ON12
        src/platform/direct2d/direct2d_entry.cpp)

# 图一乐
set(SRC_PLATFORM_GDIPLUS
        src/platform/direct2d/direct2d_platform.cpp
        src/platform/direct2d/direct2d_core.cpp
        src/platform/direct2d/direct2d_log.cpp
        src/platform/gdiplus/gdiplus_platform.cpp
        src/platform/gdiplus/gdiplus_graphics.cpp
        src/platform/gdiplus/gdiplus_fonts.cpp
        src/platform/gdiplus/gdiplus_texture.cpp
        src/platform/gdiplus/gdiplus_gui.cpp)

set(SRC_ENTRY_GDIPLUS
        src/platform/gdiplus/gdiplus_entry.cpp)

set(SRC_AUDIO_PLATFORM_RAYLIB
        src/platform/raylib/raylib_audio.cpp)

set(SRC_AUDIO_PLATFORM_FMOD
        src/platform/fmod/fmod_audio.cpp)

set(SRC_AUDIO_PLATFORM_CRI
        src/platform/cri/cri_audio.cpp)

set(MADOKAWAII_COMMON_LIBS
        RapidJSON
        rapidjson
        yaml
        libzip::zip
        libzippp::libzippp)

function(madokawaii_apply_common_settings target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    target_include_directories(${target_name}
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            PRIVATE
            ${FAST_IO_INCLUDE_DIR})

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()
endfunction()

set(MADOKAWAII_PLATFORM_SOURCES)
set(MADOKAWAII_PLATFORM_ENTRY)
set(MADOKAWAII_PLATFORM_LIBS)
set(MADOKAWAII_PLATFORM_DEFINITIONS)
set(MADOKAWAII_PLATFORM_INCLUDE_DIRS)

if(implementer_upper STREQUAL "RAYLIB")
    set(MADOKAWAII_PLATFORM_SOURCES ${SRC_PLATFORM_RAYLIB})
    set(MADOKAWAII_PLATFORM_ENTRY ${SRC_ENTRY_RAYLIB})
elseif(implementer_upper STREQUAL "DIRECT2D")
    if(NOT WIN32)
        message(FATAL_ERROR "DIRECT2D implementer is only available on Windows")
    endif()
    if(NOT MSVC)
        message(FATAL_ERROR "Direct2D backend can only be compiled via MSVC.")
    endif()
    set(MADOKAWAII_PLATFORM_SOURCES ${SRC_PLATFORM_DIRECT2D})
    set(MADOKAWAII_PLATFORM_ENTRY ${SRC_ENTRY_DIRECT2D})
    list(APPEND MADOKAWAII_PLATFORM_DEFINITIONS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    list(APPEND MADOKAWAII_PLATFORM_LIBS
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
            gdi32)
elseif(implementer_upper STREQUAL "D3D11ON12")
    if(NOT WIN32)
        message(FATAL_ERROR "D3D11ON12 implementer is only available on Windows")
    endif()
    if(NOT MSVC)
        message(FATAL_ERROR "D3D11ON12 backend can only be compiled via MSVC.")
    endif()
    set(MADOKAWAII_PLATFORM_SOURCES ${SRC_PLATFORM_D3D11ON12})
    set(MADOKAWAII_PLATFORM_ENTRY ${SRC_ENTRY_D3D11ON12})
    list(APPEND MADOKAWAII_PLATFORM_DEFINITIONS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    list(APPEND MADOKAWAII_PLATFORM_LIBS
            d2d1
            dwrite
            windowscodecs
            d3d12
            d3d11
            dxgi
            dxguid
            ole32
            uuid
            version
            comdlg32
            shell32
            user32
            gdi32)
elseif(implementer_upper STREQUAL "GDIPLUS")
    if(NOT WIN32)
        message(FATAL_ERROR "GDIPLUS implementer is only available on Windows")
    endif()
    set(MADOKAWAII_PLATFORM_SOURCES ${SRC_PLATFORM_GDIPLUS})
    set(MADOKAWAII_PLATFORM_ENTRY ${SRC_ENTRY_GDIPLUS})
    list(APPEND MADOKAWAII_PLATFORM_DEFINITIONS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    list(APPEND MADOKAWAII_PLATFORM_LIBS
            gdiplus
            d2d1
            dwrite
            windowscodecs
            d3d11
            dxgi
            dxguid
            ole32
            uuid
            comdlg32
            shell32
            user32
            gdi32)
endif()

if(implementer_upper STREQUAL "RAYLIB" OR audio_implementer_upper STREQUAL "RAYLIB")
    find_package(raylib CONFIG REQUIRED)
    list(APPEND MADOKAWAII_PLATFORM_LIBS raylib)
endif()

if(audio_implementer_upper STREQUAL "RAYLIB")
    list(APPEND MADOKAWAII_PLATFORM_SOURCES ${SRC_AUDIO_PLATFORM_RAYLIB})
elseif(audio_implementer_upper STREQUAL "FMOD")
    include(${CMAKE_CURRENT_SOURCE_DIR}/TargetArch.cmake)
    target_architecture(output_var)
    message(STATUS "Arch: ${output_var}")

    if(output_var MATCHES x86_64)
        set(fmod_lib_arch x64)
    elseif(output_var MATCHES i386)
        set(fmod_lib_arch x86)
    elseif(output_var MATCHES arm64)
        set(fmod_lib_arch arm64)
    else()
        message(FATAL_ERROR "Unknown target architecture without FMOD support. Aborting...")
    endif()

    set(fmod_lib_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/fmod/lib/${fmod_lib_arch}/fmod_vc.lib)
    set(fmod_dll_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/fmod/lib/${fmod_lib_arch}/fmod.dll)
    set(fmod_logo_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/fmod/fmod.png)

    message(STATUS "Fmod lib path: ${fmod_lib_path}")
    message(STATUS "Fmod dll path: ${fmod_dll_path}")
    message(STATUS "Fmod logo path: ${fmod_logo_path}")

    list(APPEND MADOKAWAII_PLATFORM_SOURCES ${SRC_AUDIO_PLATFORM_FMOD})
    list(APPEND MADOKAWAII_PLATFORM_INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/fmod/inc)
    list(APPEND MADOKAWAII_PLATFORM_LIBS
            ${fmod_lib_path})
elseif(audio_implementer_upper STREQUAL "CRI")
    include(${CMAKE_CURRENT_SOURCE_DIR}/TargetArch.cmake)
    target_architecture(output_var)
    message(STATUS "Arch: ${output_var}")

    find_package(Stb REQUIRED)
    find_path(DRLIBS_INCLUDE_DIRS NAMES dr_wav.h dr_flac.h dr_mp3.h)

    if(output_var MATCHES x86_64)
        set(cri_lib_arch x64)
    else()
        message(FATAL_ERROR "Unknown target architecture without CRI ADX support. Aborting...")
    endif()

    set(cri_lib_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/cri/pc/libs/${cri_lib_arch}/cri_ware_pcx64_le_import.lib)
    set(cri_dll_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/cri/pc/libs/${cri_lib_arch}/cri_ware_pcx64_le.dll)
    set(cri_logo_path
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/cri/criware_logo01.png)

    message(STATUS "CRI ADX lib path: ${cri_lib_path}")
    message(STATUS "CRI ADX dll path: ${cri_dll_path}")
    message(STATUS "CRI ADX logo path: ${cri_logo_path}")

    list(APPEND MADOKAWAII_PLATFORM_SOURCES ${SRC_AUDIO_PLATFORM_CRI})
    list(APPEND MADOKAWAII_PLATFORM_INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/cri/pc/include
            ${Stb_INCLUDE_DIR}
            ${DRLIBS_INCLUDE_DIRS})
    list(APPEND MADOKAWAII_PLATFORM_LIBS
            ${cri_lib_path})
endif()

add_library(MadokawaiiPlatform SHARED
        ${MADOKAWAII_PLATFORM_SOURCES})
add_library(Madokawaii::Platform ALIAS MadokawaiiPlatform)

madokawaii_apply_common_settings(MadokawaiiPlatform)

set_target_properties(MadokawaiiPlatform PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(MADOKAWAII_PLATFORM_INCLUDE_DIRS)
    target_include_directories(MadokawaiiPlatform PRIVATE
            ${MADOKAWAII_PLATFORM_INCLUDE_DIRS})
endif()

if(MADOKAWAII_PLATFORM_DEFINITIONS)
    target_compile_definitions(MadokawaiiPlatform PRIVATE
            ${MADOKAWAII_PLATFORM_DEFINITIONS})
endif()

if(MADOKAWAII_PLATFORM_LIBS)
    target_link_libraries(MadokawaiiPlatform PRIVATE
            ${MADOKAWAII_PLATFORM_LIBS})
endif()

add_library(MadokawaiiApp SHARED
        ${SRC_APP})
add_library(Madokawaii::App ALIAS MadokawaiiApp)

madokawaii_apply_common_settings(MadokawaiiApp)

set_target_properties(MadokawaiiApp PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_link_libraries(MadokawaiiApp PRIVATE
        Madokawaii::Platform
        ${MADOKAWAII_COMMON_LIBS})

add_executable(Madokawaii
        ${MADOKAWAII_PLATFORM_ENTRY})

madokawaii_apply_common_settings(Madokawaii)

if(MADOKAWAII_PLATFORM_DEFINITIONS)
    target_compile_definitions(Madokawaii PRIVATE
            ${MADOKAWAII_PLATFORM_DEFINITIONS})
endif()

target_link_libraries(Madokawaii PRIVATE
        Madokawaii::App
        Madokawaii::Platform
        ${MADOKAWAII_PLATFORM_LIBS})

if(audio_implementer_upper STREQUAL "FMOD")
    add_custom_command(TARGET Madokawaii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${fmod_dll_path}
            $<TARGET_FILE_DIR:Madokawaii>)
    add_custom_command(TARGET Madokawaii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:Madokawaii>/assets
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${fmod_logo_path}
            $<TARGET_FILE_DIR:Madokawaii>/assets)
endif()

if(audio_implementer_upper STREQUAL "CRI")
    add_custom_command(TARGET Madokawaii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${cri_dll_path}
            $<TARGET_FILE_DIR:Madokawaii>)
    add_custom_command(TARGET Madokawaii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:Madokawaii>/assets
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${cri_logo_path}
            $<TARGET_FILE_DIR:Madokawaii>/assets)
endif()
