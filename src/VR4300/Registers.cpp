module VR4300:Registers;

import :COP1;
import :MMU;
import :Operation;

namespace VR4300
{
	void InitializeRegisters()
	{
		cop0_reg.SetRaw(cop0_index_index, 0x3F);
		cop0_reg.SetRaw(cop0_index_context, 0x007F'FFF0);
		cop0_reg.SetRaw(cop0_index_bad_v_addr, 0xFFFF'FFFF'FFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_cause, 0xB000'007C);
		cop0_reg.SetRaw(cop0_index_epc, 0xFFFF'FFFF'FFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_ll_addr, 0xFFFF'FFFF);
		cop0_reg.SetRaw(cop0_index_watch_lo, 0xFFFF'FFFB);
		cop0_reg.SetRaw(cop0_index_watch_hi, 0xF);
		cop0_reg.SetRaw(cop0_index_x_context, 0xFFFF'FFFF'FFFF'FFF0);
		cop0_reg.SetRaw(cop0_index_error_epc, 0xFFFF'FFFF'FFFF'FFFF);
	}


	void COP0Registers::OnWriteToCause()
	{
		CheckInterrupts();
	}


	void COP0Registers::OnWriteToCompare()
	{
		cop0_reg.cause.ip &= ~0x80; /* TODO: not sure if anything else needs to be done? */
	}


	void COP0Registers::OnWriteToStatus()
	{
		fpu_is_enabled = cop0_reg.status.cu1;
		SetActiveVirtualToPhysicalFunctions();
		CheckInterrupts();
	}


	void COP0Registers::OnWriteToWired()
	{
		random_generator.SetLowerBound(cop0_reg.wired.value);
	}


	u64 COP0Registers::Get(size_t reg_index) const
	{
		auto StructToInt = [](auto struct_) {
			if constexpr (sizeof(struct_) == 4) {
				u32 ret;
				std::memcpy(&ret, &struct_, 4);
				return ret;
			}
			else if constexpr (sizeof(struct_) == 8) {
				u64 ret;
				std::memcpy(&ret, &struct_, 8);
				return ret;
			}
			else {
				static_assert(AlwaysFalse<sizeof(struct_)>, "Incorrectly sized struct given.");
			}
		};

		switch (reg_index) {
		case cop0_index_index: return StructToInt(index);
		case cop0_index_random: return random_generator.Generate(); /* Generate a random number in the interval [wired, 31] */
		case cop0_index_entry_lo_0: return StructToInt(entry_lo_0);
		case cop0_index_entry_lo_1: return StructToInt(entry_lo_1);
		case cop0_index_context: return StructToInt(context);
		case cop0_index_page_mask: return StructToInt(page_mask);
		case cop0_index_wired: return StructToInt(wired);
		case cop0_index_bad_v_addr: return StructToInt(bad_v_addr);
		case cop0_index_count: return u32(count.value >> 1); /* See the declaration of 'count' */
		case cop0_index_entry_hi: return StructToInt(entry_hi);
		case cop0_index_compare: return u32(compare.value >> 1); /* See the declaration of 'compare' */
		case cop0_index_status: return StructToInt(status);
		case cop0_index_cause: return StructToInt(cause);
		case cop0_index_epc: return StructToInt(epc);
		case cop0_index_pr_id: return StructToInt(pr_id);
		case cop0_index_config: return StructToInt(config);
		case cop0_index_ll_addr: return StructToInt(ll_addr);
		case cop0_index_watch_lo: return StructToInt(watch_lo);
		case cop0_index_watch_hi: return StructToInt(watch_hi);
		case cop0_index_x_context: return StructToInt(x_context);
		case cop0_index_parity_error: return StructToInt(parity_error);
		case cop0_index_tag_lo: return StructToInt(tag_lo);
		case cop0_index_tag_hi: return StructToInt(tag_hi);
		case cop0_index_error_epc: return StructToInt(error_epc);
		default: return 0;
		}
	}


	void FCR31::Set(u32 data)
	{
		*this = std::bit_cast<FCR31>(data);
		auto new_rounding_mode = [&] {
			switch (rm) {
			case 0b00: return FE_TONEAREST;  /* RN */
			case 0b01: return FE_TOWARDZERO; /* RZ */
			case 0b10: return FE_UPWARD;     /* RP */
			case 0b11: return FE_DOWNWARD;   /* RM */
			default: assert(false); return 0;
			}
		}();
		std::fesetround(new_rounding_mode);
	}


	u32 FCR31::Get() const
	{
		return std::bit_cast<u32>(*this);
	}


	u32 FPUControl::Get(size_t index) const
	{
		static constexpr u32 fcr0 = 0; /* TODO */
		if (index == 31) {
			return fcr31.Get();
		}
		else if (index == 0) {
			return fcr0;
		}
		else {
			return 0; /* Only #0 and #31 are "valid". */
		}
	}


	void FPUControl::Set(size_t index, u32 data)
	{
		if (index == 31) {
			fcr31.Set(data);
		}
	}


	template<FPUNumericType T>
	T FGR::Get(size_t index) const
	{
		if constexpr (std::is_same_v<T, s32>) {
			return s32(fpr[index]);
		}
		if constexpr (std::is_same_v<T, f32>) {
			return std::bit_cast<f32>(s32(fpr[index]));
		}
		if constexpr (std::is_same_v<T, s64>) {
			if (cop0_reg.status.fr) {
				return fpr[index];
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				return fpr[aligned_index] & 0xFFFF'FFFF | fpr[aligned_index + 1] << 32;
			}
		}
		if constexpr (std::is_same_v<T, f64>) {
			if (cop0_reg.status.fr) {
				return std::bit_cast<f64>(fpr[index]);
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				return std::bit_cast<f64, s64>(fpr[aligned_index] & 0xFFFF'FFFF | fpr[aligned_index + 1] << 32);
			}
		}
	}


	template<FPUNumericType T>
	void FGR::Set(size_t index, T data)
	{
		if constexpr (std::is_same_v<T, s32>) {
			fpr[index] = data;
		}
		if constexpr (std::is_same_v<T, f32>) {
			fpr[index] = std::bit_cast<s32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
		}
		if constexpr (std::is_same_v<T, s64>) {
			if (cop0_reg.status.fr) {
				fpr[index] = data;
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				fpr[aligned_index] = data & 0xFFFFFFFF;
				fpr[aligned_index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		if constexpr (std::is_same_v<T, f64>) {
			if (cop0_reg.status.fr) {
				fpr[index] = std::bit_cast<s64, f64>(data);
			}
			else {
				/* If the index is odd, then the result is undefined.
				 The only way I can get PeterLemons's fpu tests to pass is to add 1 to the index if it is odd. */
				auto aligned_index = (index + (index & 1)) & 0x1F;
				s64 conv = std::bit_cast<s64>(data);
				fpr[aligned_index] = conv & 0xFFFFFFFF;
				fpr[aligned_index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
	}


	template s32 FGR::Get<s32>(size_t) const;
	template s64 FGR::Get<s64>(size_t) const;
	template f32 FGR::Get<f32>(size_t) const;
	template f64 FGR::Get<f64>(size_t) const;

	template void FGR::Set<s32>(size_t, s32);
	template void FGR::Set<s64>(size_t, s64);
	template void FGR::Set<f32>(size_t, f32);
	template void FGR::Set<f64>(size_t, f64);
}