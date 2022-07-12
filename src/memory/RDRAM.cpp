module RDRAM;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RDRAM
{
	void AllocateExpansionPackRam()
	{
		rdram.resize(rdram_expanded_size);
		std::fill(rdram.begin() + rdram_standard_size, rdram.end(), 0);
		Memory::ReloadPageTables();
	}


	void DeallocateExpansionPackRam()
	{
		rdram.resize(rdram_standard_size);
		Memory::ReloadPageTables();
	}


	size_t GetNumberOfBytesUntilMemoryEnd(const u32 start_addr)
	{
		return std::max(std::size_t(0), rdram.size() - start_addr);
	}


	u8* GetPointerToMemory(const u32 addr)
	{
		if (addr >= rdram_standard_size && rdram.size() == rdram_standard_size) {
			return nullptr;
		}
		else {
			return rdram.data() + addr;
		}
	}


	/* $0000'0000 - $0003F'FFFF */
	template<std::integral Int>
	Int ReadStandardRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		Int ret;
		std::memcpy(&ret, rdram.data() + addr, sizeof(Int));
		return std::byteswap(ret);
	}


	/* $0040'0000 - $007F'FFFF */
	template<std::integral Int>
	Int ReadExpandedRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		if (rdram.size() == rdram_expanded_size) {
			Int ret;
			std::memcpy(&ret, rdram.data() + addr, sizeof(Int));
			return std::byteswap(ret);
		}
		else {
			return Int(0);
		}
	}


	/* $03F0'0000 - $03FF'FFFF */
	template<std::integral Int>
	Int ReadRegisterRegion(const u32 addr)
	{ /* CPU precondition: addr is always aligned */
		/* TODO */
		return Int(0);
	}


	/* $0000'0000 - $0003F'FFFF */
	template<size_t number_of_bytes>
	void WriteStandardRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		data = std::byteswap(data);
		std::memcpy(rdram.data() + addr, &data, number_of_bytes);
	}


	/* $0040'0000 - $007F'FFFF */
	template<size_t number_of_bytes>
	void WriteExpandedRegion(const u32 addr, auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		if (rdram.size() == rdram_expanded_size) {
			data = std::byteswap(data);
			std::memcpy(rdram.data() + addr, &data, number_of_bytes);
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