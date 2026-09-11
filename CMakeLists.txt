# ── Black Hole Simulator ─────────────────────────────────────────────
# Top-level build definition.

cmake_minimum_required(VERSION 3.21)

project(black_hole_sim
    VERSION 0.1.0
    DESCRIPTION "A from-scratch real-time black hole simulator"
    LANGUAGES CXX)

# ── Language settings ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies (provided by MSYS2 pacman) ──────────────────────────
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# ── The executable ───────────────────────────────────────────────────
# Add new .cpp files here as created (src/core, src/render, etc.)
add_executable(BlackHoleSim
    src/main.cpp
    src/core/Window.cpp
    src/core/Shader.cpp
    src/core/Camera.cpp
)

# Let code do  #include "core/Window.hpp"  from anywhere under src/.
target_include_directories(BlackHoleSim PRIVATE src)

# Absolute path to runtime assets (shaders, textures), baked in at compile time.
# Lets the exe find its shaders from any working directory, and lets one edit a
# shader and re-run without rebuilding (the path points at the source tree).
target_compile_definitions(BlackHoleSim PRIVATE
    BHS_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")

target_link_libraries(BlackHoleSim PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    OpenGL::GL
)

# ── Tests ────────────────────────────────────────────────────────────
# Math that doesn't need a window gets its own small executable. Run with:
#   ctest --test-dir build/mingw-debug --output-on-failure
enable_testing()

add_executable(camera_tests
    tests/camera_tests.cpp
    src/core/Camera.cpp
)
target_include_directories(camera_tests PRIVATE src)
target_link_libraries(camera_tests PRIVATE glm::glm)
add_test(NAME camera_tests COMMAND camera_tests)

# Turn on compiler warnings
foreach(target BlackHoleSim camera_tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
