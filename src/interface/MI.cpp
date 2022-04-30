module MI;

import HostSystem;
import Memory;
import VR4300;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace MI
{
	void Initialize()
	{
		mi.mode = mi.interrupt = mi.mask = 0;
		mi.version = 0x02010202;
	}


	template<InterruptType interrupt_type>
	void SetInterruptFlag()
	{
		mi.interrupt |= std::to_underlying(interrupt_type);
		if (mi.interrupt & mi.mask)
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptFlag()
	{
		mi.interrupt &= ~std::to_underlying(interrupt_type);
		if (!(mi.interrupt & mi.mask))
		{
			VR4300::ClearInterruptPending<VR4300::ExternalInterruptSource::MI>(); /* TODO: not sure if should be called */
		}
	}


	template<InterruptType interrupt_type>
	void SetInterruptMask()
	{
		mi.mask |= std::to_underlying(interrupt_type);
		if (mi.interrupt & mi.mask)
		{
			VR4300::SetInterruptPending<VR4300::ExternalInterruptSource::MI>();
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptMask()
	{
		mi.mask &= ~std::to_underlying(interrupt_type);
		if (!(mi.interrupt & mi.mask))
		{
			VR4300::ClearInterruptPending<VR4300::ExternalInterruptSource::MI>(); /* TODO: not sure if should be called */
		}
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		const u32 offset = (addr & 0xF) >> 2;
		s32 ret;
		std::memcpy(&ret, (s32*)(&mi) + offset, 4);
		return Int(ret);
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = (addr & 0xF) >> 2;
		const auto word = static_cast<s32>(data);

		static constexpr u32 offset_mode = 0;
		static constexpr u32 offset_version = 1;
		static constexpr u32 offset_interrupt = 2;
		static constexpr u32 offset_mask = 3;

		if (offset == offset_mode)
		{
			mi.mode = word;
		}
		else if (offset == offset_mask)
		{
			static constexpr s32 clear_sp_mask = 1 << 0;
			static constexpr s32   set_sp_mask = 1 << 1;
			static constexpr s32 clear_si_mask = 1 << 2;
			static constexpr s32   set_si_mask = 1 << 3;
			static constexpr s32 clear_ai_mask = 1 << 4;
			static constexpr s32   set_ai_mask = 1 << 5;
			static constexpr s32 clear_vi_mask = 1 << 6;
			static constexpr s32   set_vi_mask = 1 << 7;
			static constexpr s32 clear_pi_mask = 1 << 8;
			static constexpr s32   set_pi_mask = 1 << 9;
			static constexpr s32 clear_dp_mask = 1 << 10;
			static constexpr s32   set_dp_mask = 1 << 11;

			/* TODO: unclear what would happen if two adjacent bits would be set */
			if (word & clear_sp_mask)
				ClearInterruptMask<InterruptType::SP>();
			else if (word & set_sp_mask)
				SetInterruptMask<InterruptType::SP>();

			if (word & clear_si_mask)
				ClearInterruptMask<InterruptType::SI>();
			else if (word & set_si_mask)
				SetInterruptMask<InterruptType::SI>();

			if (word & clear_ai_mask)
				ClearInterruptMask<InterruptType::AI>();
			else if (word & set_ai_mask)
				SetInterruptMask<InterruptType::AI>();

			if (word & clear_vi_mask)
				ClearInterruptMask<InterruptType::VI>();
			else if (word & set_vi_mask)
				SetInterruptMask<InterruptType::VI>();

			if (word & clear_pi_mask)
				ClearInterruptMask<InterruptType::PI>();
			else if (word & set_pi_mask)
				SetInterruptMask<InterruptType::PI>();

			if (word & clear_dp_mask)
				ClearInterruptMask<InterruptType::DP>();
			else if (word & set_dp_mask)
				SetInterruptMask<InterruptType::DP>();
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32);

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