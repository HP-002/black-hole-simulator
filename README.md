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
An `OpenGL <version>` line on startup means the environment works.

Save one frame of the 3D view and quit:
```sh
./build/mingw-debug/BlackHoleSim.exe --screenshot out.png
```

Tests:
```sh
ctest --test-dir build/mingw-debug --output-on-failure
```

## Controls
| Key | Action |
|---|---|
| 1 / 2 | 3D view / 2D view |
| Up / Down | Grow / shrink the black hole |
| Esc | Quit |

3D view:
| Key | Action |
|---|---|
| W / A / S / D | Fly forward / left / back / right |
| Space / Left Shift | Fly up / down |
| Mouse | Look around |
| E / Q | Raise / lower exposure |
| R | Render scale 1x / 0.5x (faster) |
| P | Pause / resume the disk |

2D view:
| Key | Action |
|---|---|
| W / S | Move the light beam up / down |

## Status
Phase 4: rendering pipeline. A compute shader traces the rays into an HDR
(RGBA16F) image. Bloom (bright-pass plus a chain of half-size blurs) adds glow,
then an ACES filmic curve maps it to the screen. The window title shows FPS,
render scale and exposure. The disk's gas swirls at the Keplerian rate for each
radius, so the inner edge laps the outer. The physics is written up in
[docs/physics-notes.md](docs/physics-notes.md).

Phase 3: 3D view. One light ray per pixel is bent around the black hole on the
GPU (same integrator as the 2D view) against a procedural star-field skybox.
The camera is an observer hovering in place, so the shadow has the physical
size: sin(a) = (b_crit / r) sqrt(1 - rs / r), with b_crit ≈ 2.6 rs.

A thin accretion disk (3 to 12 rs) glows with the thin-disk temperature profile.
Its color and brightness include Doppler and gravitational shifts (brightness
~ g^4), so the side moving toward the camera is brighter. Lensing shows the far
side of the disk arching over the shadow.

Phase 2: 2D lensing. A beam of light rays bends around a Schwarzschild black
hole (top-down view). Yellow rays escape, red rays fall in. The blue ring is the
photon sphere (1.5 rs). White rays hit the step limit while orbiting.
