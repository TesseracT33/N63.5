export module RSP:VectorUnit;

import NumericalTypes;

import <algorithm>;
import <array>;
import <bit>;
import <cassert>;
import <cstring>;
import <concepts>;
import <cstring>;

import <emmintrin.h>;
import <immintrin.h>;
import <smmintrin.h>;
import <tmmintrin.h>;

/* https://github.com/rasky/r64emu/blob/master/doc/rsp.md
   https://n64brew.dev/wiki/Reality_Signal_Processor
*/

namespace RSP
{
	enum class VectorInstruction
	{
		/* Load instructions */
		LBV, LSV, LLV, LDV, LQV, LRV, LTV, LPV, LUV,

		/* Store instructions*/
		SBV, SSV, SLV, SDV, SQV, SRV, STV, SPV, SUV,

		/* Move instructions*/
		MTC2, MFC2, CTC2, CFC2,

		/* Single-lane instructions */
		VMOV, VRCP, VRSQ, VRCPH, VRSQH, VRCPL, VRSQL, VRNDN, VRNDP, VNOP, VNULL,

		/* Computational instructions */
		VMULF, VMULU, VMULQ, VMUDL, VMUDM, VMUDN, VMUDH, VMACF, VMACU, VMACQ, VMADL, VMADM, VADMN, VADMH,
		VADD, VABS, VADDC, VSUB, VSUBC, VMADN, VMADH, VSAR, VAND, VNAND, VOR, VNOR, VXOR, VNXOR,

		/* Select instructions */
		VLT, VEQ, VNE, VGE, VCH, VCR, VCL, VMRG
	};

	template<VectorInstruction instr> void VectorLoad(u32 instr_code);
	template<VectorInstruction instr> void VectorStore(u32 instr_code);
	template<VectorInstruction instr> void Move(u32 instr_code);
	template<VectorInstruction instr> void SingleLaneInstr(u32 instr_code);
	template<VectorInstruction instr> void ComputeInstr(u32 instr_code);
	template<VectorInstruction instr> void SelectInstr(u32 instr_code);

	void AddToAccumulator(__m128i low);
	void AddToAccumulator(__m128i low, __m128i mid);
	void AddToAccumulator(__m128i low, __m128i mid, __m128i high);
	void AddToAccumulatorFromMid(__m128i mid, __m128i high);
	__m128i ClampSigned(__m128i low, __m128i high);
	__m128i ClampUnsigned(__m128i low, __m128i high);
	__m128i GetVTBroadcast(int vt, int element);

	struct Accumulator
	{
		__m128i low;
		__m128i mid;
		__m128i high;
	} accumulator{};

	struct ControlRegister
	{
		__m128i low;
		__m128i high;
	};

	std::array<ControlRegister, 3> control_reg{};

	std::array<__m128i, 32> vpr{}; /* SIMD registers; eight 16-bit lanes */

	s16 div_out, div_in;
}