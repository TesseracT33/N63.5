export module VR4300:COP1;

import <cfenv>;
import <cmath>;
import <concepts>;
import <type_traits>;

import NumericalTypes;

namespace VR4300
{
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

	/* COP1/FPU instructions */
	template<FPU_Instruction instr> void FPU_Load(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Store(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Move(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Convert(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Compute(const u32 instr_code);
	template<FPU_Instruction instr> void FPU_Branch(const u32 instr_code);
	void FPU_Compare(const u32 instr_code);
}