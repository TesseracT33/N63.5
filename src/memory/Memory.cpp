module Memory;

import AI;
import Cartridge;
import DebugOptions;
import Logging;
import MI;
import PI;
import PIF;
import RDRAM;
import RI;
import RDP;
import RSP;
import SI;
import VI;
import VR4300;

namespace Memory
{
#define READ_INTERFACE(INTERFACE, INT, ADDR) [&] {                         \
if constexpr (sizeof(INT) == 4) {                                          \
	if constexpr (log_cpu_memory) {                                        \
		io_location = #INTERFACE;                                          \
	}                                                                      \
	return INTERFACE::ReadReg(ADDR);                                       \
}                                                                          \
else {                                                                     \
	Logging::LogMisc(std::format(                                          \
		"Attempted to read IO region at address ${:08X} for sized int {}", \
		ADDR, sizeof(INT)));                                               \
	return INT{};                                                          \
}                                                                          \
}()


#define WRITE_INTERFACE(INTERFACE, NUM_BYTES, ADDR, DATA)                   \
if constexpr (NUM_BYTES == 4) {                                             \
	if constexpr (log_cpu_memory) {                                         \
		io_location = #INTERFACE;                                           \
	}                                                                       \
	INTERFACE::WriteReg(ADDR, DATA);                                        \
}                                                                           \
else {                                                                      \
	Logging::LogMisc(std::format(                                           \
		"Attempted to write IO region at address ${:08X} for sized int {}", \
		ADDR, NUM_BYTES));                                                  \
}

	void Initialize()
	{
		ReloadPageTables();
	}


	template<std::signed_integral Int, Memory::Operation operation>
	Int ReadPhysical(u32 addr)
	{ /* Precondition: 'addr' is aligned according to the size of 'Int' */
		u32 page = addr >> 16;
		u8* ptr = read_page_table[page];
		Int value = [&] {
			if (ptr) {
				Int ret;
				std::memcpy(&ret, ptr + (addr & 0xFFFF), sizeof(Int));
				return std::byteswap(ret);
			}
			else if (page >= 0x3F0 && page <= 0x4FF) {
				switch (((addr >> 20) - 0x3F) & 0xF) {
				case 0: /* $03F0'0000 - $03FF'FFFF */
					return READ_INTERFACE(RDRAM, Int, addr);

				case 1: /* $0400'0000 - $040F'FFFF */
					return RSP::ReadMemoryCpu<Int>(addr);

				case 2: /* $0410'0000 - $041F'FFFF */
					return READ_INTERFACE(RDP, Int, addr);

				case 3: /* $0420'0000 - $042F'FFFF */
					Logging::LogMisc(std::format("Unexpected cpu read to address ${:08X}", addr));
					return Int{};

				case 4: /* $0430'0000 - $043F'FFFF */
					return READ_INTERFACE(MI, Int, addr);

				case 5: /* $0440'0000 - $044F'FFFF */
					return READ_INTERFACE(VI, Int, addr);

				case 6: /* $0450'0000 - $045F'FFFF */
					return READ_INTERFACE(AI, Int, addr);

				case 7: /* $0460'0000 - $046F'FFFF */
					return READ_INTERFACE(PI, Int, addr);

				case 8: /* $0470'0000 - $047F'FFFF */
					return READ_INTERFACE(RI, Int, addr);

				case 9: /* $0480'0000 - $048F'FFFF */
					return READ_INTERFACE(SI, Int, addr);

				default: /* $0490'0000 - $04EF'FFFF */
					Logging::LogMisc(std::format("Unexpected cpu read to address ${:08X}", addr));
					return Int{};
				}
			}
			else {
				Logging::LogMisc(std::format("Unexpected cpu read to address ${:08X}", addr));
				return Int{};
			}
		}(); 
		if constexpr (operation == Memory::Operation::InstrFetch) {
			if constexpr (log_cpu_instructions) {
				VR4300::last_instr_fetch_phys_addr = addr;
			}
		}
		else {
			if constexpr (log_cpu_memory) {
				if (addr >= 0x0430'0000 && addr < 0x0490'0000) {
					Logging::LogIORead(addr, value, io_location);
				}
				else if constexpr (cpu_memory_logging_mode == MemoryLoggingMode::All) {
					Logging::LogMemoryRead(addr, value);
				}
			}
		}
		return value;
	}


	void ReloadPageTables()
	{
		uint page = 0;
		for (u8*& ptr : read_page_table) {
			ptr = [&] {
				if (page <= 0x003F)                        return RDRAM::GetPointerToMemory(page << 16);
				else if (page >= 0x0400 && page <= 0x0403) return RSP::GetPointerToMemory(page << 16);
				else if (page >= 0x0800 && page <= 0x0FFF) return Cartridge::GetPointerToSRAM(page << 16);
				else if (page >= 0x1000 && page <= 0x1FBF) return Cartridge::GetPointerToROM(page << 16);
				else if (page == 0x1FC0)                   return PIF::GetPointerToMemory(page << 16);
				else                                       return (u8*)nullptr;
			}();
			++page;
		}
		page = 0;
		for (u8*& ptr : write_page_table) {
			ptr = [&] {
				if (page <= 0x003F)                        return RDRAM::GetPointerToMemory(page << 16);
				else if (page >= 0x0400 && page <= 0x0403) return RSP::GetPointerToMemory(page << 16);
				else if (page >= 0x0800 && page <= 0x0FFF) return Cartridge::GetPointerToSRAM(page << 16);
				else                                       return (u8*)nullptr;
			}();
			++page;
		}
	}


	template<size_t num_bytes>
	void WritePhysical(u32 addr, std::signed_integral auto data)
	{ /* Precondition: 'addr' is aligned according to the size of 'Int' */
		u32 page = addr >> 16;
		u8* ptr = write_page_table[page];
		if (ptr) {
			data = std::byteswap(data);
			std::memcpy(ptr + (addr & 0xFFFF), &data, num_bytes);
		}
		else if (page >= 0x3F0 && page <= 0x4FF) {
			switch (((addr >> 20) - 0x3F) & 0xF) {
			case 0: /* $03F0'0000 - $03FF'FFFF */
				WRITE_INTERFACE(RDRAM, num_bytes, addr, data); break;

			case 1: /* $0400'0000 - $040F'FFFF */
				RSP::WriteMemoryCpu<num_bytes>(addr, data); break;

			case 2: /* $0410'0000 - $041F'FFFF */
				WRITE_INTERFACE(RDP, num_bytes, addr, data); break;

			case 3: /* $0420'0000 - $042F'FFFF */
				Logging::LogMisc(std::format("Unexpected cpu write to address ${:08X}", addr)); break;

			case 4: /* $0430'0000 - $043F'FFFF */
				WRITE_INTERFACE(MI, num_bytes, addr, data); break;

			case 5: /* $0440'0000 - $044F'FFFF */
				WRITE_INTERFACE(VI, num_bytes, addr, data); break;

			case 6: /* $0450'0000 - $045F'FFFF */
				WRITE_INTERFACE(AI, num_bytes, addr, data); break;

			case 7: /* $0460'0000 - $046F'FFFF */
				WRITE_INTERFACE(PI, num_bytes, addr, data); break;

			case 8: /* $0470'0000 - $047F'FFFF */
				WRITE_INTERFACE(RI, num_bytes, addr, data); break;

			case 9: /* $0480'0000 - $048F'FFFF */
				WRITE_INTERFACE(SI, num_bytes, addr, data); break;

			default: /* $0490'0000 - $04EF'FFFF */
				Logging::LogMisc(std::format("Unexpected cpu write to address ${:08X}", addr)); break;
			}
		}
		else if ((addr & 0xFFFF'F800) == 0x1FC0'0000) { /* $1FC0'0000 - $1FC0'07FF */
			PIF::WriteMemory<num_bytes>(addr, data);
		}
		else {
			Logging::LogMisc(std::format("Unexpected cpu write to address ${:08X}", addr));
		}
		if constexpr (log_cpu_memory) {
			if (addr >= 0x0430'0000 && addr < 0x0490'0000) {
				Logging::LogIOWrite(addr, data, io_location);
			}
			else if constexpr (cpu_memory_logging_mode == MemoryLoggingMode::All) {
				Logging::LogMemoryWrite(addr, data);
			}
		}
	}


	template s32 ReadPhysical<s32, Memory::Operation::InstrFetch>(u32);
	template s8 ReadPhysical<s8, Memory::Operation::Read>(u32);
	template s16 ReadPhysical<s16, Memory::Operation::Read>(u32);
	template s32 ReadPhysical<s32, Memory::Operation::Read>(u32);
	template s64 ReadPhysical<s64, Memory::Operation::Read>(u32);

	template void WritePhysical<1>(u32, s8);
	template void WritePhysical<1>(u32, s16);
	template void WritePhysical<1>(u32, s32);
	template void WritePhysical<1>(u32, s64);
	template void WritePhysical<2>(u32, s16);
	template void WritePhysical<2>(u32, s32);
	template void WritePhysical<2>(u32, s64);
	template void WritePhysical<3>(u32, s32);
	template void WritePhysical<3>(u32, s64);
	template void WritePhysical<4>(u32, s32);
	template void WritePhysical<4>(u32, s64);
	template void WritePhysical<5>(u32, s64);
	template void WritePhysical<6>(u32, s64);
	template void WritePhysical<7>(u32, s64);
	template void WritePhysical<8>(u32, s64);
}