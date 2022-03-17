export module VR4300:COP1;

import <cfenv>;
import <cmath>;
import <concepts>;
import <string_view>;
import <type_traits>;

import NumericalTypes;

namespace VR4300
{
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

		/* Misc. */
		C
	};

	/* COP1/FPU instructions */
	template<COP1Instruction instr> void FPULoad(u32 instr_code);
	template<COP1Instruction instr> void FPUStore(u32 instr_code);
	template<COP1Instruction instr> void FPUMove(u32 instr_code);
	template<COP1Instruction instr> void FPUConvert(u32 instr_code);
	template<COP1Instruction instr> void FPUCompute(u32 instr_code);
	template<COP1Instruction instr> void FPUBranch(u32 instr_code);
	void FPUCompare(u32 instr_code);

	void InitializeFPU();

	bool fpu_is_enabled = false; /* Equal to bit 29 of the status register. If clear, all fpu instructions throw exceptions. */
}