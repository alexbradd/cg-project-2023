# Froggo 🐸

Froggo is a simple frogger-like game where you are the car and are trying to
avoid the pedestrians/animals. It is built upon the `seng` vulkan-based game
engine (included in this repo). For more info on the engine, see the `README`
inside the `seng` directory.

This game has been developed as the final evaluation for the Computer Graphics
course at Politecnico di Milano, a.y. 2022/2023.

## Building

Build requirements (for both application and engine):

1. CMake and a C++ compiler supporting at least C++17
2. GLFW 3.3 or newer
3. Vulkan SDK
4. `glslc`

The following dependencies will be automatically fetched by the build system:

1. `fmt` 10.1.1
2. `glm` 0.9.9.8
3. `tinyobjloader` 1.0.6
4. `stb_image`
5. `yaml-cpp` 0.8.0
