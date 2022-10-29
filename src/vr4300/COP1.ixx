export module VR4300:COP1;

import Util;

import <array>;
import <cfenv>;
import <cmath>;
import <concepts>;
import <random>;
import <string_view>;
import <type_traits>;

namespace VR4300
{
	template<typename T> concept FPUNumericType =
		std::is_same_v<f32, T> || std::is_same_v<f64, T> ||
		std::is_same_v<s32, T> || std::is_same_v<s64, T>;

	enum class COP1Instruction
	{
		/* Load/store/transfer instructions */
		LWC1, SWC1, LDC1, SDC1, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1,

		/* Conversion instructions */
		CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W,

		/* Computational instructions */
		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT,

		/* Branch instructions */
		BC1T, BC1F, BC1TL, BC1FL,

		/* Compare */
		C
	};

	/* COP1/FPU instructions */
	template<COP1Instruction> void FPULoad(u32 instr_code);
	template<COP1Instruction> void FPUStore(u32 instr_code);
	template<COP1Instruction> void FPUMove(u32 instr_code);
	template<COP1Instruction> void FPUConvert(u32 instr_code);
	template<COP1Instruction> void FPUCompute(u32 instr_code);
	template<COP1Instruction> void FPUBranch(u32 instr_code);
	void FPUCompare(u32 instr_code);

	void InitializeFPU();

	/* Floating point control register #31 */
	struct FCR31
	{
		u32 rm : 2; /* Rounding mode */

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
		u32 c : 1; /* Condition bit; set/cleared by the Compare instruction (or CTC1). */
		u32 fs : 1; /* TODO */
		u32 : 7;

		void Set(u32 data);
	} fcr31{};

	/* Floating point control registers. Only #0 and #31 are "valid", and #0 is read-only. */
	struct FPUControl
	{
		u32 Get(size_t index) const;
		void Set(size_t index, u32 data);
	} fpu_control;

	/* General-purpose floating point registers. */
	struct FGR
	{
		template<FPUNumericType T> T Get(size_t index) const;
		template<FPUNumericType T> void Set(size_t index, T data);
	private:
		std::array<s64, 32> fpr{};
	} fpr;


	/* Used to generate random numbers in the interval [wired, 31], when the 'random' register is read. */
	class RandomGenerator
	{
		std::random_device rd;  // Will be used to obtain a seed for the random number engine
		std::mt19937 gen{ rd() }; // Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<u32> distrib{ 0, 0x1F };
	public:
		u32 Generate()
		{
			return distrib(gen);
		}
		void SetLowerBound(auto min)
		{
			distrib = { min, 0x1F };
		}
	} random_generator{};

	bool fpu_is_enabled = false; /* Equal to bit 29 of the status register. If clear, all fpu instructions throw exceptions. */
}