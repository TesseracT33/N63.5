module RDRAM;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RDRAM
{
	void AllocateExpansionPackRam()
	{
		rdram.resize(rdram_expanded_size);
		std::fill(rdram.begin() + rdram_standard_size, rdram.end(), 0);
		static_assert(std::has_single_bit(rdram_expanded_size));
		rdram_read_mask = rdram_expanded_size - 1;
	}


	void DeallocateExpansionPackRam()
	{
		rdram.resize(rdram_standard_size);
		static_assert(std::has_single_bit(rdram_standard_size));
		rdram_read_mask = rdram_standard_size - 1;
	}


	/* $0000'0000 - $0x003F'FFFF */
	template<std::integral Int>
	Int ReadStandardRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		Int ret;
		std::memcpy(&ret, rdram.data() + addr, sizeof Int);
		return std::byteswap(ret);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::integral Int>
	Int ReadExpandedRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		if (rdram.size() != rdram_expanded_size)
		{
			return Int(0);
		}
		else
		{
			Int ret;
			std::memcpy(&ret, rdram.data() + addr, sizeof Int);
			return std::byteswap(ret);
		}
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::integral Int>
	Int ReadRegisterRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		/* STUB */
		return Int(0);
	}


	/* $0000'0000 - $0x003F'FFFF */
	template<std::size_t number_of_bytes>
	void WriteStandardRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		data = std::byteswap(data);
		std::memcpy(rdram.data() + addr, &data, number_of_bytes);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::size_t number_of_bytes>
	void WriteExpandedRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if (rdram.size() == rdram_expanded_size)
		{
			data = std::byteswap(data);
			std::memcpy(rdram.data() + addr, &data, number_of_bytes);
		}
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::size_t number_of_bytes>
	void WriteRegisterRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		/* TODO */
	}


	u8* GetPointer(const u32 addr)
	{
		return rdram.data() + (addr & rdram_read_mask); /* AND with either $3FFFFF or $7FFFFF. */
	}


	std::size_t GetNumberOfBytesUntilRegionEnd(const u32 start_addr)
	{
		return rdram.size() - (start_addr & rdram_read_mask);
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadStandardRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadExpandedRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadRegisterRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteStandardRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteExpandedRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteRegisterRegion, u32);
}