module MI;

import HostSystem;
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
	void Initialize()
	{
		mem.fill(0);
		static constexpr u8 rsp_version = 0x02; /* https://n64brew.dev/wiki/MIPS_Interface */
		static constexpr u8 rdp_version = 0x01;
		static constexpr u8 rac_version = 0x02;
		static constexpr u8 io_version = 0x02;
		mem[MI_VERSION] = io_version;
		mem[MI_VERSION + 1] = rac_version;
		mem[MI_VERSION + 2] = rdp_version;
		mem[MI_VERSION + 3] = rsp_version;
	}


	template<InterruptType interrupt_type>
	void SetInterruptFlag()
	{
		static constexpr u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT] |= interrupt_type_mask;
		if (mem[MI_INTERRUPT] & mem[MI_MASK])
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptFlag()
	{
		static constexpr u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT] &= ~interrupt_type_mask;
		if (!(mem[MI_INTERRUPT] & mem[MI_MASK]))
		{
			VR4300::ClearInterruptPending<VR4300::ExternalInterruptSource::MI>(); /* TODO: not sure if should be called */
		}
	}


	template<InterruptType interrupt_type>
	void SetInterruptMask()
	{
		static constexpr u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_MASK] |= interrupt_type_mask;
		if (mem[MI_INTERRUPT] & mem[MI_MASK])
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptMask()
	{
		static constexpr u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_MASK] &= ~interrupt_type_mask;
		if (!(mem[MI_INTERRUPT] & mem[MI_MASK]))
		{
			VR4300::ClearInterruptPending<VR4300::ExternalInterruptSource::MI>(); /* TODO: not sure if should be called */
		}
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0xF]);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = addr & 0xC;
		const u32 word = static_cast<u32>(data);
		if (offset == MI_MODE)
		{
			std::memcpy(&mem[MI_MODE], &word, 4);
		}
		else if (offset == MI_MASK)
		{
			/* The below values will byteswap only if host system is little endian. */
			static constexpr u32 test = std::byteswap<u32>(0);
			static constexpr s32 clear_sp_mask = Memory::ByteswapOnLittleEndian<s32>(0x01);
			static constexpr s32   set_sp_mask = Memory::ByteswapOnLittleEndian<s32>(0x02);
			static constexpr s32 clear_si_mask = Memory::ByteswapOnLittleEndian<s32>(0x04);
			static constexpr s32   set_si_mask = Memory::ByteswapOnLittleEndian<s32>(0x08);
			static constexpr s32 clear_ai_mask = Memory::ByteswapOnLittleEndian<s32>(0x10);
			static constexpr s32   set_ai_mask = Memory::ByteswapOnLittleEndian<s32>(0x20);
			static constexpr s32 clear_vi_mask = Memory::ByteswapOnLittleEndian<s32>(0x40);
			static constexpr s32   set_vi_mask = Memory::ByteswapOnLittleEndian<s32>(0x80);
			static constexpr s32 clear_pi_mask = Memory::ByteswapOnLittleEndian<s32>(0x100);
			static constexpr s32   set_pi_mask = Memory::ByteswapOnLittleEndian<s32>(0x200);
			static constexpr s32 clear_dp_mask = Memory::ByteswapOnLittleEndian<s32>(0x400);
			static constexpr s32   set_dp_mask = Memory::ByteswapOnLittleEndian<s32>(0x800);
			
			/* TODO: unclear what would happen if two adjacent bits would be set */
			if (data & clear_sp_mask)
				ClearInterruptMask<InterruptType::SP>();
			else if (data & set_sp_mask)
				SetInterruptMask<InterruptType::SP>();

			if (data & clear_si_mask)
				ClearInterruptMask<InterruptType::SI>();
			else if (data & set_si_mask)
				SetInterruptMask<InterruptType::SI>();

			if (data & clear_ai_mask)
				ClearInterruptMask<InterruptType::AI>();
			else if (data & set_ai_mask)
				SetInterruptMask<InterruptType::AI>();

			if (data & clear_vi_mask)
				ClearInterruptMask<InterruptType::VI>();
			else if (data & set_vi_mask)
				SetInterruptMask<InterruptType::VI>();

			if (data & clear_pi_mask)
				ClearInterruptMask<InterruptType::PI>();
			else if (data & set_pi_mask)
				SetInterruptMask<InterruptType::PI>();

			if (data & clear_dp_mask)
				ClearInterruptMask<InterruptType::DP>();
			else if (data & set_dp_mask)
				SetInterruptMask<InterruptType::DP>();
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32);

	template void ClearInterruptFlag<InterruptType::SP>();
	template void ClearInterruptFlag<InterruptType::SI>();
	template void ClearInterruptFlag<InterruptType::AI>();
	template void ClearInterruptFlag<InterruptType::VI>();
	template void ClearInterruptFlag<InterruptType::PI>();
	template void ClearInterruptFlag<InterruptType::DP>();
	template void SetInterruptFlag<InterruptType::SP>();
	template void SetInterruptFlag<InterruptType::SI>();
	template void SetInterruptFlag<InterruptType::AI>();
	template void SetInterruptFlag<InterruptType::VI>();
	template void SetInterruptFlag<InterruptType::PI>();
	template void SetInterruptFlag<InterruptType::DP>();
}