# Summary
An experimental, work-in-progress N64 emulator. Does not boot any games yet, but can run test roms.

# Running
The path to the rom file used is supplied as the first command-line argument.

# Compiling
C++20 is required for compiling. This project uses C++20 modules. As such, this probably only compiles on MSVC due to Clang's and GCC's incomplete implementations of modules. Current external dependencies are SDL2 (SDL-2.0.12 was used). I only supply Visual Studio solution files. The project settings were as follows:

C/C++ -- Additional Include Directories:
F:\SDKs\SDL2-2.0.12\include; F:\SDKs\glm\include; F:\SDKs\glew\include

Linker -- Additional Library Directories:
F:\SDKs\SDL2-2.0.12\lib\x64; F:\SDKs\glew\lib

Linker -- Input -- Additional Dependencies:
SDL2.lib; SDL2main.lib; glew32.lib; Opengl32.lib

In addition: SDL2.dll and glew32.dll.

# Tests
| Test  | Status |
| ----- | ------ |
| Dillon's CPU tests | :white_check_mark: |