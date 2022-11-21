export module VR4300:COP1;

import Util;

import <array>;
import <bit>;
import <cfenv>;
import <cmath>;
import <concepts>;
import <cstring>;
import <limits>;
import <type_traits>;
import <utility>;

namespace VR4300
{
	template<typename T> concept FpuNumericType =
		std::same_as<f32, T> || std::same_as<f64, T> ||
		std::same_as<s32, T> || std::same_as<s64, T>;

	enum class Cop1Instruction {
		/* Load/store/transfer instructions */
		LWC1, SWC1, LDC1, SDC1, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1, DCFC1, DCTC1,

		/* Conversion instructions */
		CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W,

		/* Computational instructions */
		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT,

		/* Branch instructions */
		BC1T, BC1F, BC1TL, BC1FL,

		/* Compare */
		C
	};

	/* Table 17.4 in VR4300 user manual by NEC; the 'fmt' instruction operand specifies in which format registers should be interpreted in.
		The below maps formats to identifiers. */
	enum FmtTypeID {
		Float32 = 16, Float64 = 17, Int32 = 20, Int64 = 21
	};

	enum class FpuException {
		InexactOp, Underflow, Overflow, DivByZero, InvalidOp, UnimplementedOp
	};

	/* COP1/FPU instructions */
	template<Cop1Instruction> void FpuLoad(u32 instr_code);
	template<Cop1Instruction> void FpuStore(u32 instr_code);
	template<Cop1Instruction> void FpuMove(u32 instr_code);
	template<Cop1Instruction> void FpuConvert(u32 instr_code);
	template<Cop1Instruction> void FpuCompute(u32 instr_code);
	template<Cop1Instruction> void FpuBranch(u32 instr_code);
	void FpuCompare(u32 instr_code);

	void ClearAllExceptions();
	template<std::floating_point Float> Float Flush(Float f);
	constexpr char FmtToChar(u32 fmt);
	void InitializeFpu();
	bool IsQuietNan(std::floating_point auto f);
	bool IsValidInput(std::floating_point auto f);
	bool IsValidOutput(std::floating_point auto& f);
	void OnInvalidFormat();
	bool SignalDivZero();
	bool SignalInexactOp();
	bool SignalInvalidOp();
	bool SignalOverflow();
	bool SignalUnderflow();
	bool SignalUnimplementedOp();
	template<bool ctc1 = false> bool TestAllExceptions();

	/* Floating point control register #31 */
	struct FCR31 {
		u32 rm : 2; /* Rounding mode */

		u32 flag_inexact : 1;
		u32 flag_underflow : 1;
		u32 flag_overflow : 1;
		u32 flag_div_zero : 1;
		u32 flag_invalid : 1;

		u32 enable_inexact : 1;
		u32 enable_underflow : 1;
		u32 enable_overflow : 1;
		u32 enable_div_zero : 1;
		u32 enable_invalid : 1;

		u32 cause_inexact : 1;
		u32 cause_underflow : 1;
		u32 cause_overflow : 1;
		u32 cause_div_zero : 1;
		u32 cause_invalid : 1;
		u32 cause_unimplemented : 1;

		u32 : 5;
		u32 c : 1; /* Condition bit; set/cleared by the Compare instruction (or CTC1). */
		u32 fs : 1; /* Flush subnormals: if set, and underflow and invalid exceptions are disabled,
				an fp operation resulting in a denormalized number does not cause the unimplemented operation to trigger. */
		u32 : 7;
	} fcr31;

	/* Floating point control registers. Only #0 and #31 are "valid", and #0 is read-only. */
	struct FPUControl {
		u32 Get(size_t index) const;
		void Set(size_t index, u32 value);
	} fpu_control;

	/* General-purpose floating point registers. */
	struct FGR {
		template<FpuNumericType T> T Get(size_t index) const;
		template<FpuNumericType T> void Set(size_t index, T data);
	private:
		std::array<s64, 32> fpr;
	} fpr;

	constexpr u32 fcr0 = 0xA00;

	constexpr std::array compare_cond_strings = {
		"F", "UN", "EQ", "UEQ", "OLT", "ULT", "OLE", "ULE", "SF", "NGLE", "SEQ", "NGL", "LT", "NGE", "LE", "NGT"
	};
}