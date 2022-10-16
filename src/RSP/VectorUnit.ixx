export module RSP:VectorUnit;

import NumericalTypes;

import <algorithm>;
import <array>;
import <bit>;
import <cassert>;
import <cstring>;
import <concepts>;
import <cstring>;
import <format>;

import <emmintrin.h>;
import <immintrin.h>;
import <smmintrin.h>;
import <tmmintrin.h>;

/* https://github.com/rasky/r64emu/blob/master/doc/rsp.md
   https://n64brew.dev/wiki/Reality_Signal_Processor
*/

namespace RSP
{
	using s8x16 = __m128i;
	using u8x16 = __m128i;
	using s16x8 = __m128i;
	using u16x8 = __m128i;
	using s32x4 = __m128i;
	using u32x4 = __m128i;
	using s64x2 = __m128i;
	using u64x2 = __m128i;

	enum class VectorInstruction
	{
		/* Load instructions */
		LBV, LSV, LLV, LDV, LQV, LRV, LTV, LPV, LUV, LHV, LFV, LWV,

		/* Store instructions*/
		SBV, SSV, SLV, SDV, SQV, SRV, STV, SPV, SUV, SHV, SFV, SWV,

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

	template<VectorInstruction> void VectorLoadStore(u32 instr_code);
	template<VectorInstruction> void Move(u32 instr_code);
	template<VectorInstruction> void SingleLaneInstr(u32 instr_code);
	template<VectorInstruction> void ComputeInstr(u32 instr_code);
	template<VectorInstruction> void SelectInstr(u32 instr_code);

	void AddToAcc(__m128i low);
	void AddToAcc(__m128i low, __m128i mid);
	void AddToAcc(__m128i low, __m128i mid, __m128i high);
	void AddToAccFromMid(__m128i mid, __m128i high);
	__m128i ClampSigned(__m128i low, __m128i high);
	template<VectorInstruction> __m128i ClampUnsigned(__m128i low, __m128i high);
	__m128i GetVTBroadcast(uint vt, uint element);

	struct Accumulator
	{
		__m128i low;
		__m128i mid;
		__m128i high;
	} acc;

	struct ControlRegister
	{
		__m128i low;
		__m128i high;
	};

	s16 div_out, div_in, div_dp;

	std::array<__m128i, 32> vpr; /* SIMD registers; eight 16-bit lanes */
	std::array<ControlRegister, 3> control_reg;
}