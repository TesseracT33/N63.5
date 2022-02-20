module VR4300:Registers;

import :COP1;
import :MMU;
import :Operation;

namespace VR4300
{
	void COP0Registers::CauseRegister::NotifyCpuAfterWrite()
	{
		CheckInterrupts();
	}


	void COP0Registers::CompareRegister::NotifyCpuAfterWrite()
	{
		cop0_reg.cause.ip &= ~0x80; /* TODO: not sure if anything else needs to be done? */
	}


	void COP0Registers::ConfigRegister::NotifyCpuAfterWrite()
	{

	}


	void COP0Registers::StatusRegister::NotifyCpuAfterWrite()
	{
		fpu_is_enabled = cop0_reg.status.cu1;
		SetActiveVirtualToPhysicalFunctions();
		CheckInterrupts();
	}


	void COP0Registers::WiredRegister::NotifyCpuAfterWrite()
	{
		random_generator.SetLowerBound(cop0_reg.wired.value);
	}


	u64 COP0Registers::Get(const size_t register_index) const
	{
		/* Non-UB type punning between a struct register and an u64. */
		auto StructToInt = [](const auto* struct_) -> u64
		{
			if constexpr (sizeof(*struct_) == 4)
			{
				u32 ret;
				std::memcpy(&ret, struct_, 4);
				return static_cast<u64>(ret);
			}
			else if constexpr (sizeof(*struct_) == 8)
			{
				u64 ret;
				std::memcpy(&ret, struct_, 8);
				return ret;
			}
			else
			{
				static_assert(false, "Incorrectly sized struct given.");
			}
		};

		switch (register_index)
		{
		case 0: return StructToInt(&index);
		case 1: return random_generator.Generate(); /* Generate a random number in the interval [wired, 31] */
		case 2: return StructToInt(&entry_lo_0);
		case 3: return StructToInt(&entry_lo_1);
		case 4: return StructToInt(&context);
		case 5: return StructToInt(&page_mask);
		case 6: return StructToInt(&wired);
		case 8: return StructToInt(&bad_v_addr);
		case 9: return u32(count.value >> 1); /* See the declaration of 'count' */
		case 10: return StructToInt(&entry_hi);
		case 11: return u32(compare.value >> 1); /* See the declaration of 'compare' */
		case 12: return StructToInt(&status);
		case 13: return StructToInt(&cause);
		case 14: return StructToInt(&epc);
		case 15: return StructToInt(&pr_id);
		case 16: return StructToInt(&config);
		case 17: return StructToInt(&LL_addr);
		case 18: return StructToInt(&watch_lo);
		case 19: return StructToInt(&watch_hi);
		case 20: return StructToInt(&x_context);
		case 26: return StructToInt(&parity_error);
		case 28: return StructToInt(&tag_lo);
		case 30: return StructToInt(&error_epc);
		default: return 0;
		}
	}


	void COP0Registers::Set(const size_t register_index, const u64 value)
	{
		/* Non-UB type punning between a struct register and an u64. */
		auto SetStructFromInt = [](auto* struct_, const u64 value) -> void 
		{
			if constexpr (sizeof(*struct_) == 4)
			{
				u32 value_to_write = static_cast<u32>(value);
				std::memcpy(struct_, &value_to_write, 4);
			}
			else if constexpr (sizeof(*struct_) == 8)
			{
				std::memcpy(struct_, &value, 8);
			}
			else
			{
				static_assert(false, "Incorrectly sized struct given.");
			}

			/* For registers that, once they have been written to, need to tell the rest of the cpu about it. */
			if constexpr (notifies_cpu_on_write<decltype(struct_)>)
				struct_.NotifyCpuAfterWrite();
		};

		auto SetStructFromIntMasked = [](auto* struct_, const u64 value, const u64 write_mask) -> void
		{
			if constexpr (sizeof(*struct_) == 4)
			{
				u32 struct_int;
				std::memcpy(&struct_int, struct_, 4);
				u32 value_to_write = u32(value & write_mask | struct_int & ~write_mask);
				std::memcpy(struct_, &value_to_write, 4);
			}
			else if constexpr (sizeof(*struct_) == 8)
			{
				u64 struct_int;
				std::memcpy(&struct_int, struct_, 8);
				u64 value_to_write = value & write_mask | struct_int & ~write_mask;
				std::memcpy(struct_, &value_to_write, 8);
			}
			else
			{
				static_assert(false, "Incorrectly sized struct given.");
			}

			if constexpr (notifies_cpu_on_write<decltype(struct_)>)
				struct_.NotifyCpuAfterWrite();
		};

		switch (register_index)
		{ /* Masks are used for bits that are non-writeable. */
		break; case 0: SetStructFromIntMasked(&index, value, 0x8000'003F);
		break; case 2: SetStructFromIntMasked(&entry_lo_0, value, 0x03FF'FFFF);
		break; case 3: SetStructFromIntMasked(&entry_lo_1, value, 0x03FF'FFFF);
		break; case 4: SetStructFromIntMasked(&context, value, 0xFFFF'FFFF'FFFF'FFF0);
		break; case 5: SetStructFromIntMasked(&page_mask, value, 0x01FF'E000);
		break; case 6: SetStructFromIntMasked(&wired, value, 0x3F);
		break; case 8: SetStructFromInt(&bad_v_addr, value);
		break; case 9: SetStructFromInt(&count, value << 1); /* See the declaration of 'count' */
		break; case 10: SetStructFromIntMasked(&entry_hi, value, 0xC000'00FF'FFFF'E0FF);
		break; case 11: SetStructFromInt(&compare, value << 1); /* See the declaration of 'compare' */
		break; case 12: SetStructFromIntMasked(&status, value, 0xFF57'FFFF); /* TODO: unsure about upper four bits */
		break; case 13: SetStructFromIntMasked(&cause, value, 0x300);
		break; case 14: SetStructFromInt(&epc, value);
		break; case 16: SetStructFromIntMasked(&config, value, 0x7F00'800F);
		break; case 17: SetStructFromInt(&LL_addr, value);
		break; case 18: SetStructFromIntMasked(&watch_lo, value, 0xFFFFFFFB);
		break; case 19: SetStructFromInt(&watch_hi, value);
		break; case 20: SetStructFromIntMasked(&x_context, value, 0xFFFF'FFFF'FFFF'FFF0);
		break; case 26: SetStructFromIntMasked(&parity_error, value, 0xFF);
		break; case 28: SetStructFromIntMasked(&tag_lo, value, 0x0FFF'FFC0);
		break; case 30: SetStructFromInt(&error_epc, value);
		}
	}


	void FCR31::Set(const u32 data)
	{
		/* TODO */
		/* after updating RM... */
		const int new_rounding_mode = [&] {
			switch (rm)
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


	u32 FCR31::Get() const
	{
		return std::bit_cast<s32, std::remove_reference_t<decltype(*this)>>(*this);
	}


	u32 FPUControl::Get(const size_t index) const
	{
		static constexpr s32 fcr0 = 0; /* TODO */
		if (index == 0)
			return fcr0;
		else if (index == 31)
			return fcr31.Get();
		else
			return 0; /* Only #0 and #31 are "valid". */
	}


	void FPUControl::Set(const size_t index, const u32 data)
	{
		if (index == 31)
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
			if (cop0_reg.status.fr)
				return fgr[index];
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return fgr[aligned_index] & 0xFFFFFFFF | fgr[aligned_index + 1] << 32;
			}
		}
		else if constexpr (std::is_same_v<FPU_NumericType, f64>)
		{
			if (cop0_reg.status.fr)
				return std::bit_cast<f64, s64>(fgr[index]);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				return std::bit_cast<f64, s64>(fgr[aligned_index] & 0xFFFFFFFF | fgr[aligned_index + 1] << 32);
			}
		}
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
			if (cop0_reg.status.fr)
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
			if (cop0_reg.status.fr)
				fgr[index] = std::bit_cast<s64, f64>(data);
			else
			{ /* If the index is odd, then the result is undefined. */
				const auto aligned_index = index & 0x1E;
				const s64 conv = std::bit_cast<s64, f64>(data);
				fgr[aligned_index] = conv & 0xFFFFFFFF;
				fgr[aligned_index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
			}
		}
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