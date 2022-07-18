# Summary
An experimental, work-in-progress N64 emulator. It can boot one game so far: Namco Museum 64:
![namco](https://thumbs2.imgbox.com/e1/e7/jorsCBuA_t.png)

# Running
The path to the rom file used is supplied as the first command-line argument. It must be in big-endian format. Optionally, the path to the PIF boot room can be supplied as the second argument.

# Dependencies
SDL2-2.0.22

# Compiling
C++20 is required for compiling. This project uses C++20 modules. As such, this currently only compiles on MSVC due to Clang's and GCC's incomplete implementations of modules.

# Host requirements
x86; SSE4.1

# Tests
| Test  | Status |
| ----- | ------ |
| Dillon's CPU tests | :heavy_check_mark: |
| Peter Lemon's CPUTest/CP0 | :heavy_check_mark: |
| Peter Lemon's CPUTest/CP1 | :x: |
| Peter Lemon's CPUTest/CPU | :x: |
| Peter Lemon's CPUTest/Exceptions | :heavy_check_mark: |
| Peter Lemon's RSPTest/CP2 | :x: |
| Peter Lemon's RSPTest/CPU | :heavy_check_mark: |
| Peter Lemon's RSPTest/DMAAlignment-SP | :x: |
| Peter Lemon's RSPTest/MEM | :heavy_check_mark: |
| Peter Lemon's RSPTest/XBUS | :x: |