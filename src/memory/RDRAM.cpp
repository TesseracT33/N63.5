module RDRAM;

import Memory;

#include "../EnumerateTemplateSpecializations.h"

namespace RDRAM
{
	size_t GetNumberOfBytesUntilMemoryEnd(u32 start_addr)
	{
		start_addr &= sizeof(rdram) - 1;
		return std::max(size_t(0), sizeof(rdram) - start_addr);
	}


	u8* GetPointerToMemory(u32 addr)
	{
		addr &= sizeof(rdram) - 1;
		return rdram + addr;
	}


	size_t GetSize()
	{
		return sizeof(rdram);
	}


	/* $0000'0000 - $0003F'FFFF */
	template<std::integral Int>
	Int ReadStandardRegion(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		Int ret;
		std::memcpy(&ret, rdram + addr, sizeof(Int));
		return std::byteswap(ret);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::integral Int>
	Int ReadExpandedRegion(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		if (sizeof(rdram) == rdram_expanded_size) {
			Int ret;
			std::memcpy(&ret, rdram + addr, sizeof(Int));
			return std::byteswap(ret);
		}
		else {
			return Int(0);
		}
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::integral Int>
	Int ReadRegisterRegion(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		/* TODO */
		return Int(0);
	}


	u64 RspReadCommandByteswapped(u32 addr)
	{
		/* addr may be misaligned */
		u64 command;
		for (int i = 0; i < 8; ++i) {
			*((u8*)(&command) + i) = rdram[(addr + 7 - i) & (sizeof(rdram) - 1)];
		}
		return command;
	}


	/* $0000'0000 - $0003F'FFFF */
	template<size_t number_of_bytes>
	void WriteStandardRegion(u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		data = std::byteswap(data);
		std::memcpy(rdram + addr, &data, number_of_bytes);
	}


	/* $0040'0000 - $007F'FFFF */
	template<size_t number_of_bytes>
	void WriteExpandedRegion(u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if (sizeof(rdram) == rdram_expanded_size) {
			data = std::byteswap(data);
			std::memcpy(rdram + addr, &data, number_of_bytes);
		}
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<size_t number_of_bytes>
	void WriteRegisterRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		/* TODO */
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadStandardRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadExpandedRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadRegisterRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteStandardRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteExpandedRegion, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteRegisterRegion, u32);
}