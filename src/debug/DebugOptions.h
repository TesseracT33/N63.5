#pragma once

//#define DEBUG

#ifdef DEBUG

#define LOG_CPU_INSTR
#define LOG_CPU_MEM
#define LOG_CPU_MEM_ONLY_IO
#define LOG_CPU_EXCEPTIONS
#define LOG_CPU_DMA

#endif

#define LOG_PATH "F:\\n64.txt"

//#define RUN_BOOT_ROM