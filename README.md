# Black Hole Simulator

A real-time, interactive black hole simulator written from scratch in **C++17 + OpenGL** — no game engine. Fly a camera around a black hole and watch light bend through curved spacetime.

This is a learning project.

## Tech stack
- **C++17**, built with **CMake** + **Ninja**
- **GLFW** (window/input), **GLEW** (GL loader), **GLM** (math)
- **GLSL** shaders (a compute shader handles the heavy geodesic math)
- Libraries installed via **MSYS2 pacman** (pre-built, MinGW-native)

## Project layout
```
src/core/      reusable plumbing (window, shader, camera, input)
src/render/    turning scene data into pixels
src/physics/   the relativity (CPU reference integrator + constants)
src/app/       wires it all together
assets/        shaders and textures (loaded at runtime)
docs/          physics notes
tests/         math verification
```

## Build & run
With the toolchain installed:
```sh
cmake --preset mingw-debug      # configures (finds GLFW/GLEW/GLM from MSYS2)
cmake --build --preset mingw-debug
./build/mingw-debug/BlackHoleSim.exe
```
A dark window and an `OpenGL <version>` line means the environment works.

## Status
Phase 0 — environment setup. The current `src/main.cpp` is a throwaway smoke test.
