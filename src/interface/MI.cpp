module MI;

import Memory;
import VR4300;

namespace MI
{
	void ClearInterruptFlag(InterruptType interrupt_type)
	{
		mi.interrupt &= ~std::to_underlying(interrupt_type);
		if (!(mi.interrupt & mi.mask)) {
			VR4300::ClearInterruptPending(VR4300::ExternalInterruptSource::MI); /* TODO: not sure if should be called */
		}
	}


	void ClearInterruptMask(InterruptType interrupt_type)
	{
		mi.mask &= ~std::to_underlying(interrupt_type);
		if (!(mi.interrupt & mi.mask)) {
			VR4300::ClearInterruptPending(VR4300::ExternalInterruptSource::MI); /* TODO: not sure if should be called */
		}
	}


	void Initialize()
	{
		mi.mode = mi.interrupt = mi.mask = 0;
		mi.version = 0x02010202;
	}


	s32 ReadReg(u32 addr)
	{
		u32 offset = (addr & 0xF) >> 2;
		s32 ret;
		std::memcpy(&ret, (s32*)(&mi) + offset, 4);
		return ret;
	}


	void SetInterruptFlag(InterruptType interrupt_type)
	{
		mi.interrupt |= std::to_underlying(interrupt_type);
		if (mi.interrupt & mi.mask) {
			VR4300::SetInterruptPending(VR4300::ExternalInterruptSource::MI);
		}
	}


	void SetInterruptMask(InterruptType interrupt_type)
	{
		mi.mask |= std::to_underlying(interrupt_type);
		if (mi.interrupt & mi.mask) {
			VR4300::SetInterruptPending(VR4300::ExternalInterruptSource::MI);
		}
	}


	void WriteReg(u32 addr, s32 data)
	{
		u32 offset = (addr & 0xF) >> 2;

		static constexpr u32 offset_mode = 0;
		static constexpr u32 offset_version = 1;
		static constexpr u32 offset_interrupt = 2;
		static constexpr u32 offset_mask = 3;

		if (offset == offset_mode) {
			mi.mode = data;
		}
		else if (offset == offset_mask) {
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
			if (data & clear_sp_mask) {
				ClearInterruptMask(InterruptType::SP);
			}
			else if (data & set_sp_mask) {
				SetInterruptMask(InterruptType::SP);
			}
			if (data & clear_si_mask) {
				ClearInterruptMask(InterruptType::SI);
			}
			else if (data & set_si_mask) {
				SetInterruptMask(InterruptType::SI);
			}
			if (data & clear_ai_mask) {
				ClearInterruptMask(InterruptType::AI);
			}
			else if (data & set_ai_mask) {
				SetInterruptMask(InterruptType::AI);
			}
			if (data & clear_vi_mask) {
				ClearInterruptMask(InterruptType::VI);
			}
			else if (data & set_vi_mask) {
				SetInterruptMask(InterruptType::VI);
			}
			if (data & clear_pi_mask) {
				ClearInterruptMask(InterruptType::PI);
			}
			else if (data & set_pi_mask) {
				SetInterruptMask(InterruptType::PI);
			}
			if (data & clear_dp_mask) {
				ClearInterruptMask(InterruptType::DP);
			}
			else if (data & set_dp_mask) {
				SetInterruptMask(InterruptType::DP);
			}
		}
	}
}