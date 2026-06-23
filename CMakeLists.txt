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
)

# Let code do  #include "core/Window.hpp"  from anywhere under src/.
target_include_directories(BlackHoleSim PRIVATE src)

target_link_libraries(BlackHoleSim PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    OpenGL::GL
)

# Turn on compiler warnings
if(MSVC)
    target_compile_options(BlackHoleSim PRIVATE /W4)
else()
    target_compile_options(BlackHoleSim PRIVATE -Wall -Wextra -Wpedantic)
endif()
