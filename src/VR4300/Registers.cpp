module VR4300:Registers;

import :COP1;
import :MMU;
import :Operation;

namespace VR4300
{
	//void StatusRegister::NotifyCpuAfterWrite()
	//{
	//	AssignActiveVirtualToPhysicalFunctions();
	//	SetNewEndianness();
	//}


	//void ConfigRegister::NotifyCpuAfterWrite()
	//{
	//	SetNewEndianness();
	//}


	u64 COP0Registers::Get(const size_t register_index) const
	{
		auto GetStructReg = [](const auto& structure) -> u64 /* Non-UB type punning between a struct register and an u32/u64. */
		{
			if constexpr (sizeof structure == sizeof u64)
				return std::bit_cast<u64, std::remove_reference_t<decltype(structure)>>(structure);
			else if constexpr (sizeof structure == sizeof u32)
				return static_cast<u64>(std::bit_cast<u32, std::remove_reference_t<decltype(structure)>>(structure));
			else
				static_assert(false, "Incorrectly sized struct given.");
		};

		switch (register_index)
		{
		case 0: return GetStructReg(index);
		case 1: return GetStructReg(random);
		case 2: return GetStructReg(entry_lo_0);
		case 3: return GetStructReg(entry_lo_1);
		case 4: return GetStructReg(context);
		case 5: return GetStructReg(page_mask);
		case 6: return wired;
		case 8: return bad_v_addr;
		case 9: return count;
		case 10: return GetStructReg(entry_hi);
		case 11: return compare;
		case 12: return GetStructReg(status);
		case 13: return GetStructReg(cause);
		case 14: return epc;
		case 15: return GetStructReg(pr_id);
		case 16: return GetStructReg(config);
		case 17: return LL_addr;
		case 18: return GetStructReg(watch_lo);
		case 19: return watch_hi_p_addr_1;
		case 20: return GetStructReg(x_context);
		case 26: return parity_err_diagnostic;
		case 28: return GetStructReg(tag_lo);
		case 30: return error_epc;
		default: return 0;
		}
	}


	void COP0Registers::Set(const size_t register_index, const u64 value)
	{
		auto SetStructReg = [](auto& structure, const u64 value) -> void /* Non-UB type punning between a struct register and an u32/u64. */
		{
			if constexpr (sizeof structure == sizeof u64)
				structure = std::bit_cast<std::remove_reference_t<decltype(structure)>, u64>(value);
			else if constexpr (sizeof structure == sizeof u32)
				structure = std::bit_cast<std::remove_reference_t<decltype(structure)>, u32>(static_cast<u32>(value));
			else
				static_assert(false, "Incorrectly sized struct given.");

			/* For registers that, once they have been written to, need to tell the rest of the cpu about it. */
			if constexpr (notifies_cpu_on_write<decltype(structure)>)
				structure.NotifyCpuAfterWrite();
		};

		switch (register_index)
		{
		break; case 0: SetStructReg(index, value & 0x800000CF);
		break; case 1: SetStructReg(random, value & 0x3F);
		break; case 2: SetStructReg(entry_lo_0, value & 0xCFFFFFFF);
		break; case 3: SetStructReg(entry_lo_1, value & 0xCFFFFFFF);
		break; case 4: SetStructReg(context, value & 0xFFFFFFF0);
		break; case 5: SetStructReg(page_mask, value & 0x01FFE000);
		break; case 6: wired = value;
		break; case 8: bad_v_addr = value;
		break; case 9: count = u32(value);
		break; case 10: SetStructReg(entry_hi, value & 0xC00000FFFFFFE0FF);
		break; case 11: compare = u32(value);
		break; case 12: SetStructReg(status, value);
		break; case 13: SetStructReg(cause, value & 0xB000FF7C);
		break; case 14: epc = value;
		break; case 16: SetStructReg(config, value & 0x7F00800F | 0xC6460);
		break; case 17: LL_addr = u32(value);
		break; case 18: SetStructReg(watch_lo, value & 0xFFFFFFFB);
		break; case 19: watch_hi_p_addr_1 = value;
		break; case 20: SetStructReg(x_context, value & 0xFFFFFFFF'FFFFFFF0);
		break; case 26: parity_err_diagnostic = value;
		break; case 28: SetStructReg(tag_lo, value & 0x0FFFFFC0);
		break; case 30: error_epc = value;
		}
	}


	void FPUControl31::Set(const u32 data)
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


	u32 FPUControl31::Get() const
	{
		return std::bit_cast<u32, std::remove_reference_t<decltype(*this)>>(*this);
	}


	u32 FPUControl::Get(const size_t index) const
	{
		if (index == 0)
			return 0;
		else if (index == 31)
			return FCR31.Get();
		else
			return 0; /* TODO ??? */
	}


	void FPUControl::Set(const size_t index, const u32 data)
	{
		if (index == 0)
			;
		else if (index == 31)
			FCR31.Set(data);
		else
			; /* TODO ??? */
	}


	template<typename FPU_NumericType>
	FPU_NumericType FPURegister::Get(const size_t index) const
	{
		if constexpr (std::is_same_v<FPU_NumericType, s32>)
			return s32(FGR[index]);
		else if constexpr (std::is_same_v<FPU_NumericType, f32>)
			return std::bit_cast<f32, s32>(s32(FGR[index]));
		else if constexpr (std::is_same_v<FPU_NumericType, s64>)
		{
			if (COP0_reg.status.FR)
				return FGR[index];
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return FGR[aligned_index] & 0xFFFFFFFF | FGR[aligned_index + 1] << 32;
			}
		}
		else if constexpr (std::is_same_v<FPU_NumericType, f64>)
		{
			if (COP0_reg.status.FR)
				return std::bit_cast<f64, s64>(FGR[index]);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return std::bit_cast<f64, s64>(FGR[aligned_index] & 0xFFFFFFFF | FGR[aligned_index + 1] << 32);
			}
		}
		else
			static_assert(false);
	}


	template<typename FPU_NumericType>
	void FPURegister::Set(const size_t index, const FPU_NumericType data)
	{
		if constexpr (std::is_same_v<FPU_NumericType, s32>)
			FGR[index] = data;
		else if constexpr (std::is_same_v<FPU_NumericType, f32>)
			FGR[index] = std::bit_cast<s32, f32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
		else if constexpr (std::is_same_v<FPU_NumericType, s64>)
		{
			if (COP0_reg.status.FR)
				FGR[index] = data;
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				FGR[aligned_index] = data & 0xFFFFFFFF;
				FGR[aligned_index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		else if constexpr (std::is_same_v<FPU_NumericType, f64>)
		{
			if (COP0_reg.status.FR)
				FGR[index] = std::bit_cast<s64, f64>(data);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				const s64 conv = std::bit_cast<s64, f64>(data);
				FGR[aligned_index] = conv & 0xFFFFFFFF;
				FGR[aligned_index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
		else
			static_assert(false);
	}


	template s32 FPURegister::Get<s32>(const size_t) const;
	template s64 FPURegister::Get<s64>(const size_t) const;
	template f32 FPURegister::Get<f32>(const size_t) const;
	template f64 FPURegister::Get<f64>(const size_t) const;

	template void FPURegister::Set<s32>(const size_t, const s32);
	template void FPURegister::Set<s64>(const size_t, const s64);
	template void FPURegister::Set<f32>(const size_t, const f32);
	template void FPURegister::Set<f64>(const size_t, const f64);
}