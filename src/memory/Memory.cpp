module Memory;

import AI;
import Cartridge;
import DebugOptions;
import Logging;
import MI;
import PI;
import PIF;
import RDRAM;
import RSP;
import SI;
import VI;
import VR4300;

#include "../EnumerateTemplateSpecializations.h"

namespace Memory
{
	void Initialize()
	{
		ReloadPageTables();
	}


	template<std::integral Int, MemoryAccess::Operation operation>
	Int ReadPhysical(const u32 physical_address)
	{
		/* Precondition: 'physical_address' is aligned according to the size of 'Int' */
		u32 page = physical_address >> 16;
		u8* ptr = read_page_table[page];
		Int value = [&] {
			if (ptr != nullptr) {
				Int ret;
				std::memcpy(&ret, ptr + (physical_address & 0xFFFF), sizeof(Int));
				return std::byteswap(ret);
			}
			else if (page >= 0x03F0 && page <= 0x04FF) {
				switch (((physical_address >> 20) - 0x03F) & 0xF) {
				case 0: /* $03F0'0000 - $03FF'FFFF */
					return Int(0);

				case 1: /* $0400'0000 - $040F'FFFF */
					return RSP::CPUReadRegister<Int>(physical_address);

				case 2: /* $0410'0000 - $041F'FFFF */
					return Int(0);

				case 3: /* $0420'0000 - $042F'FFFF */
					return Int(0);

				case 4: /* $0430'0000 - $043F'FFFF */
					if constexpr (log_cpu_memory) io_location = "MI";
					return MI::Read<Int>(physical_address);

				case 5: /* $0440'0000 - $044F'FFFF */
					if constexpr (log_cpu_memory) io_location = "VI";
					return VI::Read<Int>(physical_address);

				case 6: /* $0450'0000 - $045F'FFFF */
					if constexpr (log_cpu_memory) io_location = "AI";
					return AI::Read<Int>(physical_address);

				case 7: /* $0460'0000 - $046F'FFFF */
					if constexpr (log_cpu_memory) io_location = "PI";
					return PI::Read<Int>(physical_address);

				case 8: /* $0470'0000 - $047F'FFFF */
					if constexpr (log_cpu_memory) io_location = "RI";
					return Int(0);

				case 9: /* $0480'0000 - $048F'FFFF */
					if constexpr (log_cpu_memory) io_location = "SI";
					return SI::Read<Int>(physical_address);

				default: /* $0490'0000 - $04EF'FFFF */
					return Int(0);
				}
				return Int(0);
			}
			else {
				return Int(0);
			}
		}(); 
		if constexpr (operation == MemoryAccess::Operation::InstrFetch) {
			if constexpr (log_cpu_instructions) {
				VR4300::last_instr_fetch_phys_addr = physical_address;
			}
		}
		else {
			if constexpr (log_cpu_memory) {
				if (physical_address >= 0x0430'0000 && physical_address < 0x0490'0000) {
					Logging::LogIORead(physical_address, value, io_location);
				}
				else if constexpr (cpu_memory_logging_mode == MemoryLoggingMode::All) {
					Logging::LogMemoryRead(physical_address, value);
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


	template<size_t number_of_bytes>
	void WritePhysical(const u32 physical_address, auto data)
	{
		/* Precondition: 'physical_address' is aligned according to the size of 'Int' */
		u32 page = physical_address >> 16;
		u8* ptr = write_page_table[page];
		if (ptr != nullptr) {
			data = std::byteswap(data);
			std::memcpy(ptr + (physical_address & 0xFFFF), &data, number_of_bytes);
		}
		else if (page >= 0x03F0 && page <= 0x04FF) {
			switch (((physical_address >> 20) - 0x03F) & 0xF) {
			case 0: /* $03F0'0000 - $03FF'FFFF */
				break;

			case 1: /* $0400'0000 - $040F'FFFF */
				RSP::CPUWriteRegister<number_of_bytes>(physical_address, data);
				break;

			case 2: /* $0410'0000 - $041F'FFFF */
				break;

			case 3: /* $0420'0000 - $042F'FFFF */
				break;

			case 4: /* $0430'0000 - $043F'FFFF */
				if constexpr (log_cpu_memory) io_location = "MI";
				MI::Write<number_of_bytes>(physical_address, data);
				break;

			case 5: /* $0440'0000 - $044F'FFFF */
				if constexpr (log_cpu_memory) io_location = "VI";
				VI::Write<number_of_bytes>(physical_address, data);
				break;

			case 6: /* $0450'0000 - $045F'FFFF */
				if constexpr (log_cpu_memory) io_location = "AI";
				AI::Write<number_of_bytes>(physical_address, data);
				break;

			case 7: /* $0460'0000 - $046F'FFFF */
				if constexpr (log_cpu_memory) io_location = "PI";
				PI::Write<number_of_bytes>(physical_address, data);
				break;

			case 8: /* $0470'0000 - $047F'FFFF */
				if constexpr (log_cpu_memory) io_location = "RI";
				break;

			case 9: /* $0480'0000 - $048F'FFFF */
				if constexpr (log_cpu_memory) io_location = "SI";
				SI::Write<number_of_bytes>(physical_address, data);
				break;

			default: /* $0490'0000 - $04EF'FFFF */
				break;
			}
		}
		else if (physical_address >= 0x1FC0'0000 && physical_address <= 0x1FC0'07FF) {
			PIF::WriteMemory<number_of_bytes>(physical_address, data);
		}
		if constexpr (log_cpu_memory) {
			if (physical_address >= 0x0430'0000 && physical_address < 0x0490'0000) {
				Logging::LogIOWrite(physical_address, data, io_location);
			}
			else if constexpr (cpu_memory_logging_mode == MemoryLoggingMode::All) {
				Logging::LogMemoryWrite(physical_address, data);
			}
		}
	}


	template u8 ReadPhysical<u8, MemoryAccess::Operation::Read>(u32);
	template s8 ReadPhysical<s8, MemoryAccess::Operation::Read>(u32);
	template u16 ReadPhysical<u16, MemoryAccess::Operation::Read>(u32);
	template s16 ReadPhysical<s16, MemoryAccess::Operation::Read>(u32);
	template u32 ReadPhysical<u32, MemoryAccess::Operation::Read>(u32);
	template s32 ReadPhysical<s32, MemoryAccess::Operation::Read>(u32);
	template u64 ReadPhysical<u64, MemoryAccess::Operation::Read>(u32);
	template s64 ReadPhysical<s64, MemoryAccess::Operation::Read>(u32);
	template u32 ReadPhysical<u32, MemoryAccess::Operation::InstrFetch>(u32);


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WritePhysical, u32)
}