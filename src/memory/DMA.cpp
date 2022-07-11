module DMA;

import Cartridge;
import DebugOptions;
import Logging;
import N64;
import PIF;
import RDRAM;
import RSP;
import VR4300;

namespace DMA
{
	template<Type type, Location src, Location dst>
	void Init(size_t length, s32 src_start_addr, s32 dst_start_addr)
	{
		u8* src_start_ptr = GetPointerFromAddress<src>(src_start_addr);
		u8* dst_start_ptr = GetPointerFromAddress<dst>(dst_start_addr);

		size_t bytes_until_source_end = GetNumberOfBytesUntilMemoryEnd<src>(src_start_addr);
		size_t bytes_until_dest_end = GetNumberOfBytesUntilMemoryEnd<dst>(dst_start_addr);
		size_t num_bytes_to_copy = std::min(length, std::min(bytes_until_source_end, bytes_until_dest_end));

		std::memcpy(dst_start_ptr, src_start_ptr, num_bytes_to_copy);

		static constexpr auto cycles_per_byte_dma = 18;
		auto cycles_until_finish = num_bytes_to_copy * cycles_per_byte_dma;
		N64::Event n64_event = [] {
			if constexpr (type == DMA::Type::PI) return N64::Event::PI_DMA_FINISH;
			else if constexpr (type == DMA::Type::SI) return N64::Event::SI_DMA_FINISH;
			else if constexpr (type == DMA::Type::SP) return N64::Event::SP_DMA_FINISH;
			else static_assert("Unknown DMA type given as argument to \"DMA::Init\"");
		}();
		N64::EnqueueEvent(n64_event, cycles_until_finish, VR4300::p_cycle_counter);

		if constexpr (log_dma)
		{
			std::string output = std::format("From {} ${:X} to {} ${:X}; ${:X} bytes",
				location_to_string_table[std::to_underlying(src)], src_start_addr,
				location_to_string_table[std::to_underlying(dst)], dst_start_addr, num_bytes_to_copy);
			Logging::LogDMA(output);
		}
	}


	template<Type type, Location source, Location dest>
	void Init(size_t rows, size_t bytes_per_row, size_t skip, s32 src_start_addr, s32 dst_start_addr)
	{
		// TODO
	}


	template<Location location>
	u8* GetPointerFromAddress(u32 addr)
	{
		if constexpr (location == Location::Cartridge)
			return Cartridge::GetPointerToROM(addr);
		else if constexpr (location == Location::PIF)
			return PIF::GetPointerToRAM(addr);
		else if constexpr (location == Location::RDRAM)
			return RDRAM::GetPointerToMemory(addr);
		else
			static_assert(location != location, "Unknown location given as argument to function \"GetPointerFromAddress\"");
	}


	template<Location location>
	size_t GetNumberOfBytesUntilMemoryEnd(u32 addr)
	{
		if constexpr (location == Location::Cartridge)
			return Cartridge::GetNumberOfBytesUntilROMEnd(addr);
		else if constexpr (location == Location::PIF)
			return PIF::GetNumberOfBytesUntilRAMEnd(addr);
		else if constexpr (location == Location::RDRAM)
			return RDRAM::GetNumberOfBytesUntilMemoryEnd(addr);
		else
			static_assert(location != location, "Unknown location given as argument to function \"GetNumberOfBytesUntilRegionEnd\"");
	}


	template void Init <Type::PI, Location::Cartridge, Location::RDRAM> (size_t, s32, s32);
	template void Init <Type::PI, Location::RDRAM, Location::Cartridge>(size_t, s32, s32);
	template void Init <Type::SI, Location::PIF, Location::RDRAM>(size_t, s32, s32);
	template void Init <Type::SI, Location::RDRAM, Location::PIF>(size_t, s32, s32);
	template void Init <Type::SP, Location::SPRAM, Location::RDRAM>(size_t, size_t, size_t, s32, s32);
	template void Init <Type::SP, Location::RDRAM, Location::SPRAM>(size_t, size_t, size_t, s32, s32);
}