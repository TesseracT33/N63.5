module DMA;

import Cartridge;
import N64;
import RDRAM;
import VR4300;

import <algorithm>;

namespace DMA
{
	template<Location source, Location dest>
	void Init(const size_t length, const u32 source_start_addr, const u32 dest_start_addr)
	{
		u8* source_start_ptr = GetPointerFromAddress<source>(source_start_addr);
		u8* dest_start_ptr = GetPointerFromAddress<dest>(dest_start_addr);

		const size_t bytes_until_source_end = GetNumberOfBytesUntilRegionEnd<source>(source_start_addr);
		const size_t bytes_until_dest_end = GetNumberOfBytesUntilRegionEnd<dest>(dest_start_addr);
		const size_t number_of_bytes_to_copy = std::min(length, std::min(bytes_until_source_end, bytes_until_dest_end));

		std::memcpy(dest_start_ptr, source_start_ptr, number_of_bytes_to_copy);

		static constexpr size_t cycles_per_byte_pi_dma = 9;
		const size_t cycles_until_finish = number_of_bytes_to_copy * cycles_per_byte_pi_dma;
		N64::EnqueueEvent(N64::Event::PI_DMA_FINISH, cycles_until_finish, VR4300::p_cycle_counter);
	}


	template<Location location>
	u8* GetPointerFromAddress(const u32 addr)
	{
		if constexpr (location == Location::RDRAM)
			return RDRAM::GetPointer(addr);
		else if constexpr (location == Location::Cartridge)
			return Cartridge::GetPointerToROM(addr);
	}


	template<Location location>
	size_t GetNumberOfBytesUntilRegionEnd(const u32 addr)
	{
		if constexpr (location == Location::RDRAM)
			return RDRAM::GetNumberOfBytesUntilRegionEnd(addr);
		else if constexpr (location == Location::Cartridge)
			return Cartridge::GetNumberOfBytesUntilRegionEnd(addr);
	}


	template void Init <Location::Cartridge, Location::RDRAM> (const size_t, const u32, const u32);
	template void Init <Location::RDRAM, Location::Cartridge>(const size_t, const u32, const u32);
}