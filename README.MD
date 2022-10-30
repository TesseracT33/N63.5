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
| Suite | Passing | Notes |
| ----- | ------- | ----- |
| Dillon's CPU tests | 26/26 :heavy_check_mark: | |
| Peter Lemon's CPUTest/CP0 | 2/2 :heavy_check_mark: | |
| Peter Lemon's CPUTest/CP1 | 28/46 :x: | Most failing are in C/FPUCompare/ |
| Peter Lemon's CPUTest/CPU | 44/45 :x: | Failing: CPUTIMINGNTSC |
| Peter Lemon's CPUTest/DMAAlignment-PI-cart | 0/6 :x: | |
| Peter Lemon's CPUTest/Exceptions | 16/17 :x: | Failing: ExceptionCompareDisabled. Logs suggest that it wants cause.ip7 to be set? |
| Peter Lemon's FrameBuffer | 4/4 :heavy_check_mark: | |
| Peter Lemon's RSPTest/CP2 | 24/26 :x: | Failing: RSPCP2LWV, RSPTransposeMatrixVMOV |
| Peter Lemon's RSPTest/CP2/RESERVED | 0/12 :x: | |
| Peter Lemon's RSPTest/CPU | 14/14 :heavy_check_mark: | |
| Peter Lemon's RSPTest/DMAAlignment-SP | 0/2 :x: | |
| Peter Lemon's RSPTest/MEM | 1/1 :heavy_check_mark: | |
| Peter Lemon's RSPTest/XBUS | 0/1 :x: | |

