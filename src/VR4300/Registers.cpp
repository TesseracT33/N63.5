module VR4300:Registers;

import :COP1;
import :MMU;
import :Operation;

namespace VR4300
{
	void COP0Registers::CompareRegister::NotifyCpuAfterWrite()
	{
		cop0_reg.cause.ip7 = 0; /* TODO: not sure if anything else needs to be done? */
	}


	void COP0Registers::ConfigRegister::NotifyCpuAfterWrite()
	{

	}


	void COP0Registers::StatusRegister::NotifyCpuAfterWrite()
	{
		SetActiveVirtualToPhysicalFunctions();
		fpu_is_enabled = cop0_reg.status.CU1;
	}


	u64 COP0Registers::Get(const size_t register_index) const
	{
		auto StructToIntegral = [](const auto& struct_) -> u64 /* Non-UB type punning between a struct register and an u64. */
		{
			if constexpr (sizeof struct_ == 8)
				return std::bit_cast<u64, std::remove_reference_t<decltype(struct_)>>(struct_);
			else if constexpr (sizeof struct_ == 4)
				return static_cast<u64>(std::bit_cast<s32, std::remove_reference_t<decltype(struct_)>>(struct_));
			else
				static_assert(false, "Incorrectly sized struct given.");
		};

		switch (register_index)
		{
		case 0: return StructToIntegral(index);
		case 1: return StructToIntegral(random);
		case 2: return StructToIntegral(entry_lo_0);
		case 3: return StructToIntegral(entry_lo_1);
		case 4: return StructToIntegral(context);
		case 5: return StructToIntegral(page_mask);
		case 6: return StructToIntegral(wired);
		case 8: return StructToIntegral(bad_v_addr);
		case 9: return StructToIntegral(count);
		case 10: return StructToIntegral(entry_hi);
		case 11: return StructToIntegral(compare);
		case 12: return StructToIntegral(status);
		case 13: return StructToIntegral(cause);
		case 14: return StructToIntegral(epc);
		case 15: return StructToIntegral(pr_id);
		case 16: return StructToIntegral(config);
		case 17: return StructToIntegral(LL_addr);
		case 18: return StructToIntegral(watch_lo);
		case 19: return StructToIntegral(watch_hi);
		case 20: return StructToIntegral(x_context);
		case 26: return StructToIntegral(parity_error);
		case 28: return StructToIntegral(tag_lo);
		case 30: return StructToIntegral(error_epc);
		default: return 0;
		}
	}


	void COP0Registers::Set(const size_t register_index, const u64 value)
	{
		auto SetStructFromIntegral = [](auto& struct_, const u64 value) -> void /* Non-UB type punning between a struct register and an u32/u64. */
		{
			if constexpr (sizeof struct_ == 8)
				struct_ = std::bit_cast<std::remove_reference_t<decltype(struct_)>, u64>(value);
			else if constexpr (sizeof struct_ == 4)
				struct_ = std::bit_cast<std::remove_reference_t<decltype(struct_)>, u32>(static_cast<u32>(value));
			else
				static_assert(false, "Incorrectly sized struct given.");

			/* For registers that, once they have been written to, need to tell the rest of the cpu about it. */
			if constexpr (notifies_cpu_on_write<decltype(struct_)>)
				struct_.NotifyCpuAfterWrite();
		};

		switch (register_index)
		{
		break; case 0: SetStructFromIntegral(index, value & 0x800000CF);
		break; case 1: SetStructFromIntegral(random, value & 0x3F);
		break; case 2: SetStructFromIntegral(entry_lo_0, value & 0xCFFFFFFF);
		break; case 3: SetStructFromIntegral(entry_lo_1, value & 0xCFFFFFFF);
		break; case 4: SetStructFromIntegral(context, value & 0xFFFFFFF0);
		break; case 5: SetStructFromIntegral(page_mask, value & 0x01FFE000);
		break; case 6: SetStructFromIntegral(wired, value);
		break; case 8: SetStructFromIntegral(bad_v_addr, value);
		break; case 9: SetStructFromIntegral(count, value);
		break; case 10: SetStructFromIntegral(entry_hi, value & 0xC00000FFFFFFE0FF);
		break; case 11: SetStructFromIntegral(compare, value);
		break; case 12: SetStructFromIntegral(status, value);
		break; case 13: SetStructFromIntegral(cause, value & 0xB000FF7C);
		break; case 14: SetStructFromIntegral(epc, value);
		break; case 16: SetStructFromIntegral(config, value & 0x7F00800F | 0xC6460);
		break; case 17: SetStructFromIntegral(LL_addr, value);
		break; case 18: SetStructFromIntegral(watch_lo, value & 0xFFFFFFFB);
		break; case 19: SetStructFromIntegral(watch_hi, value);
		break; case 20: SetStructFromIntegral(x_context, value & 0xFFFFFFFF'FFFFFFF0);
		break; case 26: SetStructFromIntegral(parity_error, value);
		break; case 28: SetStructFromIntegral(tag_lo, value & 0x0FFFFFC0);
		break; case 30: SetStructFromIntegral(error_epc, value);
		}
	}


	void FCR31::Set(const s32 data)
	{
		/* TODO */
		/* after updating RM... */
		const int new_rounding_mode = [&] {
			switch (RM)
			{
			case 0b00: return FE_TONEAREST;  /* RN */
			case 0b01: return FE_TOWARDZERO; /* RZ */
			case 0b10: return FE_UPWARD;     /* RP */
			case 0b11: return FE_DOWNWARD;   /* RM */
			default: return 0; /* impossible */
			}
		}();
		std::fesetround(new_rounding_mode);
		/* TODO: initial rounding mode? */
	}


	s32 FCR31::Get() const
	{
		return std::bit_cast<u32, std::remove_reference_t<decltype(*this)>>(*this);
	}


	s32 FPUControl::Get(const size_t index) const
	{
		if (index == 0)
			return 0;
		else if (index == 31)
			return fcr31.Get();
		else
			return 0; /* TODO ??? */
	}


	void FPUControl::Set(const size_t index, const s32 data)
	{
		if (index == 0)
			;
		else if (index == 31)
			fcr31.Set(data);
	}


	template<typename FPU_NumericType>
	FPU_NumericType FGR::Get(const size_t index) const
	{
		if constexpr (std::is_same_v<FPU_NumericType, s32>)
			return s32(fgr[index]);
		else if constexpr (std::is_same_v<FPU_NumericType, f32>)
			return std::bit_cast<f32, s32>(s32(fgr[index]));
		else if constexpr (std::is_same_v<FPU_NumericType, s64>)
		{
			if (cop0_reg.status.FR)
				return fgr[index];
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return fgr[aligned_index] & 0xFFFFFFFF | fgr[aligned_index + 1] << 32;
			}
		}
		else if constexpr (std::is_same_v<FPU_NumericType, f64>)
		{
			if (cop0_reg.status.FR)
				return std::bit_cast<f64, s64>(fgr[index]);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return std::bit_cast<f64, s64>(fgr[aligned_index] & 0xFFFFFFFF | fgr[aligned_index + 1] << 32);
			}
		}
		else
			static_assert(false);
	}


	template<typename FPU_NumericType>
	void FGR::Set(const size_t index, const FPU_NumericType data)
	{
		if constexpr (std::is_same_v<FPU_NumericType, s32>)
			fgr[index] = data;
		else if constexpr (std::is_same_v<FPU_NumericType, f32>)
			fgr[index] = std::bit_cast<s32, f32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
		else if constexpr (std::is_same_v<FPU_NumericType, s64>)
		{
			if (cop0_reg.status.FR)
				fgr[index] = data;
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				fgr[aligned_index] = data & 0xFFFFFFFF;
				fgr[aligned_index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		else if constexpr (std::is_same_v<FPU_NumericType, f64>)
		{
			if (cop0_reg.status.FR)
				fgr[index] = std::bit_cast<s64, f64>(data);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				const s64 conv = std::bit_cast<s64, f64>(data);
				fgr[aligned_index] = conv & 0xFFFFFFFF;
				fgr[aligned_index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		else
			static_assert(false);
	}


	template s32 FGR::Get<s32>(const size_t) const;
	template s64 FGR::Get<s64>(const size_t) const;
	template f32 FGR::Get<f32>(const size_t) const;
	template f64 FGR::Get<f64>(const size_t) const;

	template void FGR::Set<s32>(const size_t, const s32);
	template void FGR::Set<s64>(const size_t, const s64);
	template void FGR::Set<f32>(const size_t, const f32);
	template void FGR::Set<f64>(const size_t, const f64);
}