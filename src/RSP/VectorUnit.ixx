export module RSP:VectorUnit;

import Host;
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
		LBV, LSV, LLV, LDV, SBV, SSV, SLV, SDV, LQV, LRV, SQV, SRV, LTV, STV, LPV, LUV, SPV, SUV,

		MTC2, MFC2, CTC2, CFC2,

		VMOV, VRCP, VRSQ, VRCPH, VRSQH, VRCPL, VRSQL, VNOP, VNULL,

		VMULF, VMULU, VMUDL, VMUDM, VMUDN, VMUDH, VMACF, VMACU, VMADL, VMADM, VADMN, VADMH,
		VADD, VABS, VADDC, VSUB, VSUBC, VMADN, VMADH, VSAR, VAND, VNAND, VOR, VNOR, VXOR, VNXOR,

		VLT, VEQ, VNE, VGE, VCH, VCR, VCL, VMRG
	};

	template<VectorInstruction instr> void ScalarLoad(u32 instr_code);
	template<VectorInstruction instr> void ScalarStore(u32 instr_code);
	template<VectorInstruction instr> void VectorLoad(u32 instr_code);
	template<VectorInstruction instr> void VectorStore(u32 instr_code);
	template<VectorInstruction instr> void Move(u32 instr_code);
	template<VectorInstruction instr> void SingleLaneInstr(u32 instr_code);
	template<VectorInstruction instr> void Compute(u32 instr_code);
	template<VectorInstruction instr> void SelectInstr(u32 instr_code);

	__m128i GetVTBroadcast(int vt, int element);

	struct Accumulator
	{
		__m128i low;
		__m128i mid;
		__m128i hi;
	} accumulator{};

	struct ControlRegister
	{
		__m128i low;
		__m128i high;
	} vc0{}, vcc{}, vce{};

	constexpr std::array control_reg_ptrs = { &vc0, &vcc, &vce };

	std::array<__m128i, 32> vpr{}; /* SIMD registers; eight 16-bit lanes */

	u16 div_out, div_in;
}