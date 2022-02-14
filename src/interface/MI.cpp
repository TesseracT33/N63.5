module MI;

import Memory;
import VR4300;

#include "../Utils/EnumerateTemplateSpecializations.h"

/* Register addresses (offsets; repeat every 16 bytes starting from $0430'0000) */
#define MI_MODE      0x0 /* R/W */
#define MI_VERSION   0x4 /* R   */
#define MI_INTERRUPT 0x8 /* R   */
#define MI_MASK      0xC /* R/W */

namespace MI
{
	template<InterruptType interrupt_type>
	void SetInterruptFlag()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT] |= interrupt_type_mask;
		if (mem[MI_INTERRUPT] & mem[MI_MASK] & interrupt_type_mask)
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptFlag()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT] &= ~interrupt_type_mask;
		/* TODO: Call VR4300::ClearInterruptPending if !(mem[MI_INTERRUPT] & mem[MI_MASK] & interrupt_type_mask) ??? */
	}


	template<InterruptType interrupt_type>
	void SetInterruptMask()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_MASK] |= interrupt_type_mask;
		if (mem[MI_INTERRUPT] & mem[MI_MASK] & interrupt_type_mask)
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptMask()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT] &= ~interrupt_type_mask;
	}


	void WriteToMiMode0(const u8 data)
	{
		mem[MI_MODE] = data;
		/* todo */
	}


	void WriteToMiMode1(const u8 data)
	{
		mem[MI_MODE + 1] = mem[MI_MODE + 1] & 0xC0 | data & 0x3F;
		/* todo */
	}


	void WriteToMask0(const u8 data)
	{
		/* TODO: unclear what would happen if two adjacent bits would be set */
		if (data & 0x01)
			ClearInterruptMask<InterruptType::SP>();
		else if (data & 0x02)
			SetInterruptMask<InterruptType::SP>();

		if (data & 0x04)
			ClearInterruptMask<InterruptType::SI>();
		else if (data & 0x08)
			SetInterruptMask<InterruptType::SI>();

		if (data & 0x10)
			ClearInterruptMask<InterruptType::AI>();
		else if (data & 0x20)
			SetInterruptMask<InterruptType::AI>();

		if (data & 0x40)
			ClearInterruptMask<InterruptType::VI>();
		else if (data & 0x80)
			SetInterruptMask<InterruptType::VI>();
	}


	void WriteToMask1(const u8 data)
	{
		if (data & 0x01)
			ClearInterruptMask<InterruptType::PI>();
		else if (data & 0x02)
			SetInterruptMask<InterruptType::PI>();

		if (data & 0x04)
			ClearInterruptMask<InterruptType::DP>();
		else if (data & 0x08)
			SetInterruptMask<InterruptType::DP>();
	}


	void Initialize()
	{
		mem.fill(0);
		constexpr static u8 rsp_version = 0x02; /* https://n64brew.dev/wiki/MIPS_Interface */
		constexpr static u8 rdp_version = 0x02;
		constexpr static u8 rac_version = 0x01;
		constexpr static u8 io_version = 0x02;
		mem[MI_VERSION] = io_version;
		mem[MI_VERSION + 2] = rac_version;
		mem[MI_VERSION + 3] = rdp_version;
		mem[MI_VERSION + 4] = rsp_version; /* TODO: endianness? */
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0xF]);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		switch (addr & 0xF)
		{
		case MI_MODE:
			WriteToMiMode0(u8(data));
			if constexpr (number_of_bytes > 1)
				WriteToMiMode1(u8(data >> 8));
			break;

		case MI_MODE + 1:
			WriteToMiMode1(u8(data));
			break;

		case MI_VERSION + 1:
			if constexpr (number_of_bytes == 8)
				WriteToMask0(u8(data >> 56));
			break;

		case MI_VERSION + 2:
			if constexpr (number_of_bytes >= 7)
				WriteToMask0(u8(data >> 48));
			if constexpr (number_of_bytes == 8)
				WriteToMask1(u8(data >> 56));
			break;

		case MI_VERSION + 3:
			if constexpr (number_of_bytes >= 6)
				WriteToMask0(u8(data >> 40));
			if constexpr (number_of_bytes >= 7)
				WriteToMask1(u8(data >> 48));
			break;

		case MI_INTERRUPT:
			if constexpr (number_of_bytes >= 5)
				WriteToMask0(u8(data >> 32));
			if constexpr (number_of_bytes >= 6)
				WriteToMask1(u8(data >> 40));
			break;

		case MI_INTERRUPT + 1:
			if constexpr (number_of_bytes >= 4)
				WriteToMask0(u8(data >> 24));
			if constexpr (number_of_bytes >= 5)
				WriteToMask1(u8(data >> 32));
			break;

		case MI_INTERRUPT + 2:
			if constexpr (number_of_bytes >= 3)
				WriteToMask0(u8(data >> 16));
			if constexpr (number_of_bytes >= 4)
				WriteToMask1(u8(data >> 24));
			break;

		case MI_INTERRUPT + 3:
			if constexpr (number_of_bytes >= 2)
				WriteToMask0(u8(data >> 8));
			if constexpr (number_of_bytes >= 3)
				WriteToMask1(u8(data >> 16));
			break;

		case MI_MASK:
			WriteToMask0(u8(data));
			if constexpr (number_of_bytes >= 2)
				WriteToMask1(u8(data >> 8));
			break;

		case MI_MASK + 1:
			WriteToMask1(u8(data));
			break;

		default:
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32);
}