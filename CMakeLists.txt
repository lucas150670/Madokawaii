cmake_minimum_required(VERSION 3.31)
project(Madokawaii)

# configured with vcpkg
find_package(raylib CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(yaml CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(Madokawaii
        src/app/main.cpp
        src/app/chart.cpp
        src/platform/raylib/raylib_graphics.cpp
        src/platform/raylib/raylib_entry.cpp
        src/platform/raylib/raylib_core.cpp
        src/app/app_config.cpp
        src/app/line_update.cpp
        src/app/line_renderer.cpp
        src/platform/raylib/raylib_log.cpp
        src/platform/raylib/raylib_audio.cpp
        src/platform/raylib/raylib_fonts.cpp
        src/platform/raylib/raylib_texture.cpp)
target_include_directories(Madokawaii PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Madokawaii PRIVATE raylib RapidJSON rapidjson yaml)