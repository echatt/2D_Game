# Unfrogged
A shrimple template project for OpenGL development. The main goal is to make it easier for beginners to start by eliminating most of the CMake setup and dependency fetching from the equation.

The default application draws an orange window with a single ImGui widget displaying some text.

The following libraries are fetched by CMake (the commands to fetch these libraries can be found in `external/CMakeLists.txt`):
- [GLFW](https://github.com/glfw/glfw) (window creation and input handling)
- [GLM](https://github.com/g-truc/glm) (3D vector and matrix math)
- [Dear ImGui](https://github.com/ocornut/imgui) (simple immediate-mode GUI)
- [Tracy](https://github.com/wolfpld/tracy) (profiling)

Furthermore, the following libraries are vendored (included as part of this repository in `vendor/`)
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (loading common image formats)
- [stb_include](https://github.com/nothings/stb/blob/master/stb_include.h) (`#include` support for arbitrary [in particular GLSL] files. Notably, the vendored version is not the same as the official version)
- [glad 2](https://github.com/Dav1dde/glad) (loading OpenGL function pointers)
