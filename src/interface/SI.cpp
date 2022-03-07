module SI;

import Memory;
import MI;

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
			mem[SI_STATUS + 2] |= status_flag_mask;
		else
			mem[SI_STATUS + 3] |= status_flag_mask;
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		constexpr u8 status_flag_mask = static_cast<u8>(status_flag);
		if constexpr (status_flag == StatusFlag::INTERRUPT)
			mem[SI_STATUS + 2] &= ~status_flag_mask;
		else
			mem[SI_STATUS + 3] &= ~status_flag_mask;
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x1F]);  /* TODO: number of register bytes is 0x1C.. */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = addr & 0x1C;
		const auto word = static_cast<u32>(data);
		switch (offset)
		{
		case SI_DRAM_ADDR:
			/* TODO */
			break;

		case SI_PIF_ADDR_RD64B:
			/* TODO */
			break;

		case SI_PIF_ADDR_WR4B:
			/* TODO */
			break;

		case SI_PIF_ADDR_WR64B:
			/* TODO */
			break;

		case SI_PIF_ADDR_RD4B:
			/* TODO */
			break;

		case SI_STATUS:
			/* Writing any value to SI_STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			mem[SI_STATUS + 2] &= ~0x10;
			MI::ClearInterruptFlag<MI::InterruptType::SI>();
			// TODO: RCP flag
			break;

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32)
}