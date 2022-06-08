module DMA;

import Cartridge;
import DebugOptions;
import Logging;
import N64;
import PIF;
import RDRAM;
import VR4300;

namespace DMA
{
	template<Type type, Location source, Location dest>
	void Init(const std::size_t length, const s32 source_start_addr, const s32 dest_start_addr)
	{
		u8* source_start_ptr = GetPointerFromAddress<source>(source_start_addr);
		u8* dest_start_ptr = GetPointerFromAddress<dest>(dest_start_addr);

		const std::size_t bytes_until_source_end = GetNumberOfBytesUntilMemoryEnd<source>(source_start_addr);
		const std::size_t bytes_until_dest_end = GetNumberOfBytesUntilMemoryEnd<dest>(dest_start_addr);
		const std::size_t number_of_bytes_to_copy = std::min(length, std::min(bytes_until_source_end, bytes_until_dest_end));

		std::memcpy(dest_start_ptr, source_start_ptr, number_of_bytes_to_copy);

		static constexpr auto cycles_per_byte_dma = 18;
		const auto cycles_until_finish = number_of_bytes_to_copy * cycles_per_byte_dma;
		const N64::Event n64_event = [] {
			if constexpr (type == DMA::Type::PI) return N64::Event::PI_DMA_FINISH;
			else if constexpr (type == DMA::Type::SI) return N64::Event::SI_DMA_FINISH;
			else static_assert("Unknown DMA type given as argument to \"DMA::Init\"");
		}();
		N64::EnqueueEvent(n64_event, cycles_until_finish, VR4300::p_cycle_counter);

		if constexpr (log_dma)
		{
			const std::string output = std::format("From {} ${:X} to {} ${:X}; ${:X} bytes",
				LocationToString(source), source_start_addr, LocationToString(dest), dest_start_addr, number_of_bytes_to_copy);
			Logging::LogDMA(output);
		}
	}


	template<Location location>
	u8* GetPointerFromAddress(const u32 addr)
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
	std::size_t GetNumberOfBytesUntilMemoryEnd(const u32 addr)
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


	constexpr std::string_view LocationToString(Location loc)
	{
		switch (loc)
		{
		case Location::Cartridge: return "CART";
		case Location::PIF: return "PIF";
		case Location::RDRAM: return "RDRAM";
		default: assert(false); return "";
		}
	}


	template void Init <Type::PI, Location::Cartridge, Location::RDRAM> (std::size_t, s32, s32);
	template void Init <Type::PI, Location::RDRAM, Location::Cartridge>(std::size_t, s32, s32);
	template void Init <Type::SI, Location::PIF, Location::RDRAM>(std::size_t, s32, s32);
	template void Init <Type::SI, Location::RDRAM, Location::PIF>(std::size_t, s32, s32);
}