module SI;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

#define SI_DRAM_ADDR      0x00
#define SI_PIF_ADDR_RD64B 0x04
#define SI_PIF_ADDR_WR4B  0x08
#define SI_PIF_ADDR_WR64B 0x10
#define SI_PIF_ADDR_RD4B  0x14
#define SI_STATUS         0x18

namespace SI
{
	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		constexpr u8 status_flag_mask = static_cast<u8>(status_flag);
		if constexpr (status_flag == StatusFlag::INTERRUPT)
			mem[SI_STATUS + 1] |= status_flag_mask;
		else
			mem[SI_STATUS] |= status_flag_mask;
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		constexpr u8 status_flag_mask = static_cast<u8>(status_flag);
		if constexpr (status_flag == StatusFlag::INTERRUPT)
			mem[SI_STATUS + 1] &= ~status_flag_mask;
		else
			mem[SI_STATUS] &= ~status_flag_mask;
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr % mem.size()]);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		switch (addr % mem.size())
		{
		case SI_DRAM_ADDR:
			Memory::GenericWrite
				<number_of_bytes <= 3 ? number_of_bytes : 3>(&mem[SI_DRAM_ADDR], data);
			break;

		case SI_DRAM_ADDR + 1:
			Memory::GenericWrite
				<number_of_bytes <= 2 ? number_of_bytes : 2>(&mem[SI_DRAM_ADDR + 1], data);
			break;

		case SI_DRAM_ADDR + 2:
			Memory::GenericWrite<1>(&mem[SI_DRAM_ADDR + 2], data);
			break;

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}