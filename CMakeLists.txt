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
# Add new .cpp files here
add_executable(BlackHoleSim
    src/main.cpp
    src/core/Window.cpp
    src/core/Shader.cpp
    src/core/Camera.cpp
    src/render/Mesh2D.cpp
    src/render/StarField.cpp
    src/render/Cubemap.cpp
    src/render/FullscreenTriangle.cpp
    src/physics/Schwarzschild.cpp
    src/app/Lensing2D.cpp
    src/app/BlackHole3D.cpp
)

# Includes are relative to src/
target_include_directories(BlackHoleSim PRIVATE src)

# Absolute assets path: works from any cwd, shader edits need no rebuild
target_compile_definitions(BlackHoleSim PRIVATE
    BHS_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")

target_link_libraries(BlackHoleSim PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    OpenGL::GL
)

# ── Tests ────────────────────────────────────────────────────────────
# Window-free math tests. Run with:
#   ctest --test-dir build/mingw-debug --output-on-failure
enable_testing()

add_executable(camera_tests
    tests/camera_tests.cpp
    src/core/Camera.cpp
)
target_include_directories(camera_tests PRIVATE src)
target_link_libraries(camera_tests PRIVATE glm::glm)
add_test(NAME camera_tests COMMAND camera_tests)

add_executable(schwarzschild_tests
    tests/schwarzschild_tests.cpp
    src/physics/Schwarzschild.cpp
)
target_include_directories(schwarzschild_tests PRIVATE src)
target_link_libraries(schwarzschild_tests PRIVATE glm::glm)
add_test(NAME schwarzschild_tests COMMAND schwarzschild_tests)

add_executable(starfield_tests
    tests/starfield_tests.cpp
    src/render/StarField.cpp
)
target_include_directories(starfield_tests PRIVATE src)
target_link_libraries(starfield_tests PRIVATE glm::glm)
add_test(NAME starfield_tests COMMAND starfield_tests)

# Turn on compiler warnings
foreach(target BlackHoleSim camera_tests schwarzschild_tests
        starfield_tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
