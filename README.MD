# Summary
An experimental, work-in-progress N64 emulator.

# Compiling and running
C++20 is required for compiling. Current external dependencies are SDL2 (SDL-2.0.12 was used). I only supply Visual Studio solution files. The project settings were as follows:

C/C++ -- Additional Include Directories:
F:\SDKs\SDL2-2.0.12\include

Linker -- Additional Library Directories:
F:\SDKs\SDL2-2.0.12\lib\x64;

Linker -- Input -- Additional Dependencies:
SDL2.lib; SDL2main.lib;