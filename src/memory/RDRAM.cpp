module RDRAM;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RDRAM
{
	void AllocateExpansionPackRam()
	{
		RDRAM.resize(rdram_expanded_size);
		std::fill(RDRAM.begin() + rdram_standard_size, RDRAM.end(), 0);
	}


	/* $0000'0000 - $0x003F'FFFF */
	template<std::integral Int>
	Int ReadStandardRegion(const u32 address)
	{
		return Memory::GenericRead<Int>(&RDRAM[address]);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::integral Int>
	Int ReadExpandedRegion(const u32 address)
	{
		if (RDRAM.size() != rdram_expanded_size)
			return Int(0);
		else
			return Memory::GenericRead<Int>(&RDRAM[address]);
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::integral Int>
	Int ReadRegisterRegion(const u32 address)
	{
		assert(false);
		return Int(0);
	}


	/* $0000'0000 - $0x003F'FFFF */
	template<std::size_t number_of_bytes>
	void WriteStandardRegion(const u32 address, const auto data)
	{
		Memory::GenericWrite<number_of_bytes>(&RDRAM[address], data);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::size_t number_of_bytes>
	void WriteExpandedRegion(const u32 address, const auto data)
	{
		if (RDRAM.size() == rdram_expanded_size)
			return;
		else
			Memory::GenericWrite<number_of_bytes>(&RDRAM[address], data);
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::size_t number_of_bytes>
	void WriteRegisterRegion(const u32 address, const auto data)
	{
		assert(false);
	}


	u8* GetPointer(const u32 addr)
	{
		return RDRAM.data() + (addr & (RDRAM.size() - 1)); /* AND with either $3FFFFF or $7FFFFF. */
	}


	std::size_t GetNumberOfBytesUntilRegionEnd(const u32 start_addr)
	{
		return RDRAM.size() - (start_addr & (RDRAM.size() - 1));
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadStandardRegion, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadExpandedRegion, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadRegisterRegion, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteStandardRegion, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteExpandedRegion, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteRegisterRegion, const u32);
}