export module VR4300:COP1;

import <array>;
import <cfenv>;
import <cmath>;
import <concepts>;
import <limits>;
import <type_traits>;

import :COP0;

import NumericalTypes;

namespace VR4300
{
	template<typename T>
	concept FPU_NumericType =
		std::is_same_v<f32, typename std::remove_cv<T>::type> ||
		std::is_same_v<f64, typename std::remove_cv<T>::type> ||
		std::is_same_v<s32, typename std::remove_cv<T>::type> ||
		std::is_same_v<s64, typename std::remove_cv<T>::type>;

	enum class FPU_Instruction
	{
		/* Load/store/transfer instructions */
		LWC1, SWC1, LDC1, SDC1, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1,

		/* Conversion instructions */
		CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W,

		/* Computational instructions */
		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT,

		/* Branch instructions */
		BC1T, BC1F, BC1TL, BC1FL
	};

	struct FPU_Control_31
	{
		void Set(const u32 data)
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

		u32 Get() const { return std::bit_cast<u32, FPU_Control_31>(*this); }

		u32 RM : 2;

		u32 flag_I : 1; /* Inexact Operation */
		u32 flag_U : 1; /* Underflow */
		u32 flag_O : 1; /* Overflow */
		u32 flag_Z : 1; /* Division by Zero */
		u32 flag_V : 1; /* Invalid Operation */

		u32 enable_I : 1;
		u32 enable_U : 1;
		u32 enable_O : 1;
		u32 enable_Z : 1;
		u32 enable_V : 1;

		u32 cause_I : 1;
		u32 cause_U : 1;
		u32 cause_O : 1;
		u32 cause_Z : 1;
		u32 cause_V : 1;
		u32 cause_E : 1; /* Unimplemented Operation */

		u32 : 5;
		u32 C : 1;
		u32 FS : 1;
		u32 : 7;
	} FCR31{};

	struct FloatingPointGeneralPurposeRegisters
	{
		template<typename FPU_NumericType>
		inline FPU_NumericType Get(const size_t index) const
		{
			if constexpr (std::is_same<FPU_NumericType, s32>::value)
				return s32(FGR[index]);
			else if constexpr (std::is_same<FPU_NumericType, s64>::value)
				return CP0_reg.status.FR ? FGR[index] : FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32; /* TODO: if index is odd, result is supposed to be undefined. If index == 31, then that is very bad */
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
				return std::bit_cast<f32, s32>(s32(FGR[index]));
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
				return CP0_reg.status.FR ? std::bit_cast<f64, s64>(FGR[index])
				: std::bit_cast<f64, s64>(FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32);
			else
				static_assert(false);
		}

		template<typename FPU_NumericType>
		inline void Set(const size_t index, const FPU_NumericType data)
		{
			if constexpr (std::is_same<FPU_NumericType, s32>::value)
			{
				FGR[index] = data;
			}
			else if constexpr (std::is_same<FPU_NumericType, s64>::value)
			{
				if (FCR31.FS)
					FGR[index] = data;
				else
				{
					FGR[index] = data & 0xFFFFFFFF;
					FGR[index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
				}
			}
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
			{
				FGR[index] = std::bit_cast<s32, f32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
			}
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
			{
				if (FCR31.FS)
					FGR[index] = std::bit_cast<s64, f64>(data);
				else
				{
					const s64 conv = std::bit_cast<s64, f64>(data);
					FGR[index] = conv & 0xFFFFFFFF;
					FGR[index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
				}
			}
			else
				static_assert(false);
		}

	private:
		std::array<s64, 32> FGR{};
	} FGR;

	struct FloatingPointControlRegisters
	{
		u32 Get(const size_t index) const
		{
			if (index == 0)
				return 0;
			else if (index == 31)
				return FCR31.Get();
			else
				return 0; /* TODO ??? */
		}

		void Set(const size_t index, const u32 data)
		{
			if (index == 0)
				;
			else if (index == 31)
				FCR31.Set(data);
			else
				; /* TODO ??? */
		}
	} FPU_control;

	/* COP1/FPU instructions */
	template<FPU_Instruction instr> void FPU_Load(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Store(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Move(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Convert(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Compute(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Branch(const u32 instr_code);
	void FPU_Compare(const u32 instr_code);
}