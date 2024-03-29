module MI;

import BuildOptions;
import Log;
import VR4300;

namespace MI
{
	void CheckInterrupts()
	{
		if (mi.interrupt & mi.mask) {
			VR4300::SetInterruptPending(VR4300::ExternalInterruptSource::MI);
		}
		else {
			VR4300::ClearInterruptPending(VR4300::ExternalInterruptSource::MI);
		}
	}


	void ClearInterruptFlag(InterruptType interrupt_type)
	{
		mi.interrupt &= ~std::to_underlying(interrupt_type);
		CheckInterrupts();
	}


	void Initialize()
	{
		mi.mode = mi.interrupt = mi.mask = 0;
		mi.version = 0x0202'0102;
	}


	s32 ReadReg(u32 addr)
	{
		static_assert(sizeof(mi) >> 2 == 4);
		u32 offset = addr >> 2 & 3;
		s32 ret;
		std::memcpy(&ret, (s32*)(&mi) + offset, 4);
		if constexpr (log_io_mi) {
			Log::IoRead("MI", RegOffsetToStr(offset), ret);
		}
		return ret;
	}


	constexpr std::string_view RegOffsetToStr(u32 reg_offset)
	{
		switch (reg_offset) {
		case Mode: return "MI_MODE";
		case Version: return "MI_VERSION";
		case Interrupt: return "MI_INTERRUPT";
		case Mask: return "MI_MASK";
		default: std::unreachable();
		}
	}


	void SetInterruptFlag(InterruptType interrupt_type)
	{
		mi.interrupt |= std::to_underlying(interrupt_type);
		CheckInterrupts();
	}


	void WriteReg(u32 addr, s32 data)
	{
		static_assert(sizeof(mi) >> 2 == 4);
		u32 offset = addr >> 2 & 3;
		if constexpr (log_io_ai) {
			Log::IoWrite("MI", RegOffsetToStr(offset), data);
		}

		if (offset == Register::Mode) {
			mi.mode = data;
			if (mi.mode & 0x800) {
				ClearInterruptFlag(InterruptType::DP);
			}
		}
		else if (offset == Register::Mask) {
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

			auto ClearMask = [&](InterruptType interrupt_type) {
				mi.mask &= ~std::to_underlying(interrupt_type);
			};

			auto SetMask = [&](InterruptType interrupt_type) {
				mi.mask |= std::to_underlying(interrupt_type);
			};

			/* TODO: unclear what would happen if two adjacent bits would be set */
			if (data & clear_sp_mask) {
				ClearMask(InterruptType::SP);
			}
			else if (data & set_sp_mask) {
				SetMask(InterruptType::SP);
			}
			if (data & clear_si_mask) {
				ClearMask(InterruptType::SI);
			}
			else if (data & set_si_mask) {
				SetMask(InterruptType::SI);
			}
			if (data & clear_ai_mask) {
				ClearMask(InterruptType::AI);
			}
			else if (data & set_ai_mask) {
				SetMask(InterruptType::AI);
			}
			if (data & clear_vi_mask) {
				ClearMask(InterruptType::VI);
			}
			else if (data & set_vi_mask) {
				SetMask(InterruptType::VI);
			}
			if (data & clear_pi_mask) {
				ClearMask(InterruptType::PI);
			}
			else if (data & set_pi_mask) {
				SetMask(InterruptType::PI);
			}
			if (data & clear_dp_mask) {
				ClearMask(InterruptType::DP);
			}
			else if (data & set_dp_mask) {
				SetMask(InterruptType::DP);
			}

			CheckInterrupts();
		}
	}
}